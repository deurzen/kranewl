#include <trace.hh>

#include <kranewl/model.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg-view.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <sys/types.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/box.h>
#include <wlr/util/edges.h>
}
#undef static
#undef namespace
#undef class

View::View(
    XDGView_ptr,
    Uid uid,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    struct wlr_surface* wlr_surface
)
    : Node(Type::XDGShell, uid),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(nullptr),
      mp_context(nullptr),
      mp_workspace(nullptr),
      mp_wlr_surface(wlr_surface),
      m_alpha(1.f),
      m_resize(0),
      m_tile_decoration(FREE_DECORATION),
      m_free_decoration(FREE_DECORATION),
      m_active_decoration(FREE_DECORATION),
      m_minimum_dim({}),
      m_preferred_dim({}),
      m_free_region({}),
      m_tile_region({}),
      m_active_region({}),
      m_prev_region({}),
      m_inner_region({}),
      m_last_cursor_pos({}),
      m_activated(false),
      m_focused(false),
      m_mapped(false),
      m_managed(true),
      m_free(false),
      m_urgent(false),
      m_floating(false),
      m_fullscreen(false),
      m_scratchpad(false),
      m_contained(false),
      m_invincible(false),
      m_sticky(false),
      m_iconifyable(true),
      m_iconified(false),
      m_disowned(false),
      m_scene_layer(SCENE_LAYER_NONE),
      m_last_focused(std::chrono::steady_clock::now()),
      m_last_touched(std::chrono::steady_clock::now()),
      m_managed_since(std::chrono::steady_clock::now()),
      m_outside_state(OutsideState::Unfocused)
{
    wl_signal_init(&m_events.unmap);
}

#ifdef XWAYLAND
View::View(
    XWaylandView_ptr,
    Uid uid,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    struct wlr_surface* wlr_surface
)
    : Node(Type::XWaylandManaged, uid),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(nullptr),
      mp_context(nullptr),
      mp_workspace(nullptr),
      mp_wlr_surface(wlr_surface),
      m_alpha(1.f),
      m_resize(0),
      m_tile_decoration(FREE_DECORATION),
      m_free_decoration(FREE_DECORATION),
      m_active_decoration(FREE_DECORATION),
      m_minimum_dim({}),
      m_preferred_dim({}),
      m_free_region({}),
      m_tile_region({}),
      m_active_region({}),
      m_prev_region({}),
      m_inner_region({}),
      m_activated(false),
      m_focused(false),
      m_mapped(false),
      m_managed(true),
      m_urgent(false),
      m_floating(false),
      m_fullscreen(false),
      m_scratchpad(false),
      m_contained(false),
      m_invincible(false),
      m_sticky(false),
      m_iconifyable(true),
      m_iconified(false),
      m_disowned(false),
      m_scene_layer(SCENE_LAYER_NONE),
      m_last_focused(std::chrono::steady_clock::now()),
      m_last_touched(std::chrono::steady_clock::now()),
      m_managed_since(std::chrono::steady_clock::now()),
      m_outside_state(OutsideState::Unfocused)
{
    wl_signal_init(&m_events.unmap);
}
#endif

View::~View()
{}

void
View::set_activated(bool activated)
{
    m_activated = activated;
}

void
View::set_focused(bool focused)
{
    auto now = std::chrono::steady_clock::now();
    m_last_touched = now;
    m_last_focused = now;
    m_focused = focused;

    if (focused) {
        m_urgent = false;

        switch (m_outside_state) {
        case OutsideState::Unfocused:         m_outside_state = OutsideState::Focused;         return;
        case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::FocusedDisowned; return;
        case OutsideState::UnfocusedSticky:   m_outside_state = OutsideState::FocusedSticky;   return;
        default: return;
        }
    } else {
        switch (m_outside_state) {
        case OutsideState::Focused:         m_outside_state = OutsideState::Unfocused;         return;
        case OutsideState::FocusedDisowned: m_outside_state = OutsideState::UnfocusedDisowned; return;
        case OutsideState::FocusedSticky:   m_outside_state = OutsideState::UnfocusedSticky;   return;
        default: return;
        }
    }
}

void
View::set_mapped(bool mapped)
{
    m_mapped = mapped;
}

void
View::set_managed(bool managed)
{
    m_managed = managed;
}

void
View::set_free(bool free)
{
    m_free = free;
}

void
View::set_urgent(bool urgent)
{
    m_urgent = urgent;
}

void
View::set_floating(bool floating)
{
    m_floating = floating;
}

void
View::set_fullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

void
View::set_scratchpad(bool scratchpad)
{
    m_scratchpad = scratchpad;
}

void
View::set_contained(bool contained)
{
    m_contained = contained;
}

void
View::set_invincible(bool invincible)
{
    m_invincible = invincible;
}

void
View::set_sticky(bool sticky)
{
    m_sticky = sticky;

    switch (sticky) {
    case true:
        switch (m_outside_state) {
        case OutsideState::Focused:   m_outside_state = OutsideState::FocusedSticky;   return;
        case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedSticky; return;
        default: return;
        }
    case false:
        switch (m_outside_state) {
        case OutsideState::FocusedSticky:   m_outside_state = OutsideState::Focused;   return;
        case OutsideState::UnfocusedSticky: m_outside_state = OutsideState::Unfocused; return;
        default: return;
        }
    }
}

void
View::set_iconifyable(bool iconifyable)
{
    m_iconifyable = iconifyable;
}

void
View::set_iconified(bool iconified)
{
    m_iconified = iconified;
}

void
View::set_disowned(bool disowned)
{
    m_disowned = disowned;

    switch (disowned) {
    case true:
        switch (m_outside_state) {
        case OutsideState::Focused:   m_outside_state = OutsideState::FocusedDisowned;   return;
        case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedDisowned; return;
        default: return;
        }
    case false:
        switch (m_outside_state) {
        case OutsideState::FocusedDisowned:   m_outside_state = OutsideState::Focused;   return;
        case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::Unfocused; return;
        default: return;
        }
    }
}

bool
View::belongs_to_active_track() const
{
    return m_scene_layer == mp_workspace->track_layer();
}

std::chrono::time_point<std::chrono::steady_clock>
View::last_focused() const
{
    return m_last_focused;
}

std::chrono::time_point<std::chrono::steady_clock>
View::last_touched() const
{
    return m_last_touched;
}

std::chrono::time_point<std::chrono::steady_clock>
View::managed_since() const
{
    return m_managed_since;
}

void
View::map()
{
    if (!m_mapped) {
        wlr_scene_node_set_enabled(mp_scene, true);
        m_mapped = true;
    }
}

void
View::unmap()
{
    if (m_mapped) {
        focus(Toggle::Off);
        wlr_scene_node_set_enabled(mp_scene, false);
        m_mapped = false;
    }
}

void
View::center()
{
    if (mp_output)
        mp_output->place_at_center(m_free_region);
}

void
View::tile(Toggle toggle)
{
    TRACE();

    switch (toggle) {
    case Toggle::On:
    {
        if (!m_floating)
            return;

        set_floating(false);
        if (!m_free)
            relayer(SCENE_LAYER_TILE);

        break;
    }
    case Toggle::Off:
    {
        if (m_floating)
            return;

        set_floating(true);
        if (m_free)
            relayer(SCENE_LAYER_FREE);

        break;
    }
    case Toggle::Reverse:
    {
        tile(
            floating()
            ? Toggle::On
            : Toggle::Off
        );
        return;
    }
    default: break;
    }
}

void
View::relayer(SceneLayer layer)
{
    if (layer == m_scene_layer)
        return;

    m_scene_layer = layer;
    wlr_scene_node_reparent(
        mp_scene,
        mp_server->m_scene_layers[layer]
    );
}

void
View::raise() const
{
    wlr_scene_node_raise_to_top(mp_scene);
}

void
View::lower() const
{
    wlr_scene_node_lower_to_bottom(mp_scene);
}

void
View::render_decoration()
{
    TRACE();

    Decoration const& decoration = m_active_decoration;
    ColorScheme const& colorscheme = decoration.colorscheme;
    const float* colors = nullptr;

    switch (outside_state()) {
    case OutsideState::Focused:
    {
        if (decoration.frame)
            colors = colorscheme.focused.values;

        break;
    }
    case OutsideState::FocusedDisowned:
    {
        if (decoration.frame)
            colors = colorscheme.fdisowned.values;

        break;
    }
    case OutsideState::FocusedSticky:
    {
        if (decoration.frame)
            colors = colorscheme.fsticky.values;

        break;
    }
    case OutsideState::Unfocused:
    {
        if (decoration.frame)
            colors = colorscheme.unfocused.values;

        break;
    }
    case OutsideState::UnfocusedDisowned:
    {
        if (decoration.frame)
            colors = colorscheme.udisowned.values;

        break;
    }
    case OutsideState::UnfocusedSticky:
    {
        if (decoration.frame)
            colors = colorscheme.usticky.values;

        break;
    }
    case OutsideState::Urgent:
    {
        if (decoration.frame)
            colors = colorscheme.urgent.values;

        break;
    }
    }

    if (decoration.frame)
        for (std::size_t i = 0; i < 4; ++i)
            wlr_scene_rect_set_color(
                m_protrusions[i],
                colors
            );
}

void
View::render_cycle_indicator()
{

}

static uint32_t
extents_to_wlr_edges(Extents const& extents)
{
    uint32_t wlr_edges = WLR_EDGE_NONE;

    if (extents.top)
        wlr_edges |= WLR_EDGE_TOP;

    if (extents.bottom)
        wlr_edges |= WLR_EDGE_BOTTOM;

    if (extents.left)
        wlr_edges |= WLR_EDGE_LEFT;

    if (extents.right)
        wlr_edges |= WLR_EDGE_RIGHT;

    return wlr_edges;
}

uint32_t
View::free_decoration_to_wlr_edges() const
{
    return extents_to_wlr_edges(m_free_decoration.extents());
}

uint32_t
View::tile_decoration_to_wlr_edges() const
{
    return extents_to_wlr_edges(m_tile_decoration.extents());
}

void
View::set_free_region(Region const& region)
{
    m_free_region = region;
    set_active_region(region);
}

void
View::set_free_pos(Pos const& pos)
{
    m_free_region.pos = pos;
    set_active_pos(pos);
}

void
View::set_tile_region(Region const& region)
{
    m_tile_region = region;
    set_active_region(region);
}

void
View::set_free_decoration(Decoration const& decoration)
{
    m_free_decoration = decoration;
    m_active_decoration = decoration;
}

void
View::set_tile_decoration(Decoration const& decoration)
{
    m_tile_decoration = decoration;
    m_active_decoration = decoration;
}

void
View::indicate_as_next()
{
    wlr_scene_node_set_enabled(&m_next_indicator[0]->node, true);
    wlr_scene_node_set_enabled(&m_next_indicator[1]->node, true);
}

void
View::unindicate_as_next()
{
    wlr_scene_node_set_enabled(&m_next_indicator[0]->node, false);
    wlr_scene_node_set_enabled(&m_next_indicator[1]->node, false);
}

void
View::indicate_as_prev()
{
    wlr_scene_node_set_enabled(&m_prev_indicator[0]->node, true);
    wlr_scene_node_set_enabled(&m_prev_indicator[1]->node, true);
}

void
View::unindicate_as_prev()
{
    wlr_scene_node_set_enabled(&m_prev_indicator[0]->node, false);
    wlr_scene_node_set_enabled(&m_prev_indicator[1]->node, false);
}

void
View::format_uid()
{
    std::stringstream uid_ss;
    uid_ss << "0x" << std::hex << uid() << std::dec;
    uid_ss << " [" << title();
    uid_ss << ", " << pid() << "]";
    uid_ss << " (W)";
    m_uid_formatted = uid_ss.str();
}

View::OutsideState
View::outside_state() const
{
    if (m_urgent)
        return OutsideState::Urgent;

    return m_outside_state;
}

void
View::set_active_region(Region const& region)
{
    m_prev_region = m_active_region;
    set_inner_region(region);
    m_active_region = region;
}

void
View::set_active_pos(Pos const& pos)
{
    m_prev_region = m_active_region;
    m_active_region.pos = pos;
}

void
View::set_inner_region(Region const& region)
{
    if (m_active_decoration.frame) {
        Frame const& frame = *m_active_decoration.frame;

        m_inner_region.pos.x = frame.extents.left;
        m_inner_region.pos.y = frame.extents.top;
        m_inner_region.dim.w = region.dim.w - frame.extents.left - frame.extents.right;
        m_inner_region.dim.h = region.dim.h - frame.extents.top - frame.extents.bottom;
    } else {
        m_inner_region.pos.x = 0;
        m_inner_region.pos.y = 0;
        m_inner_region.dim = region.dim;
    }
}
