#include <kranewl/tree/client.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
#ifdef XWAYLAND
#include <wlr/xwayland.h>
#endif
}
#undef static
#undef class
#undef namespace

Client::Client(
    Server_ptr server,
    Surface surface,
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace
)
    : m_uid{surface.uid()},
      mp_server{server},
      m_surface{surface},
      mp_output{output},
      mp_context{context},
      mp_workspace{workspace},
      m_free_region({}),
      m_tile_region({}),
      m_active_region({}),
      m_previous_region({}),
      m_inner_region({}),
      m_tile_decoration({}, DEFAULT_COLOR_SCHEME),
      m_free_decoration({}, DEFAULT_COLOR_SCHEME),
      m_active_decoration({}, DEFAULT_COLOR_SCHEME),
      m_focused(false),
      m_mapped(false),
      m_managed(true),
      m_urgent(false),
      m_floating(false),
      m_fullscreen(false),
      m_contained(false),
      m_invincible(false),
      m_sticky(false),
      m_iconifyable(true),
      m_iconified(false),
      m_disowned(false),
      m_producing(true),
      m_attaching(false),
      m_last_focused(std::chrono::steady_clock::now()),
      m_last_touched(std::chrono::steady_clock::now()),
      m_managed_since(std::chrono::steady_clock::now()),
      m_outside_state{OutsideState::Unfocused}
{}

Client::~Client()
{}

Client::OutsideState
Client::get_outside_state() const noexcept
{
    if (m_urgent)
        return Client::OutsideState::Urgent;

    return m_outside_state;
}

struct wlr_surface*
Client::get_surface() noexcept
{
    switch (m_surface.type) {
    case SurfaceType::XDGShell: //fallthrough
    case SurfaceType::LayerShell: return m_surface.xdg->surface;
    case SurfaceType::X11Managed: //fallthrough
    case SurfaceType::X11Unmanaged: return m_surface.xwayland->surface;
    }

    return nullptr;
}

void
Client::focus() noexcept
{
    m_focused = true;
    m_last_focused = std::chrono::steady_clock::now();

    switch (m_outside_state) {
    case OutsideState::Unfocused:         m_outside_state = OutsideState::Focused;         return;
    case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::FocusedDisowned; return;
    case OutsideState::UnfocusedSticky:   m_outside_state = OutsideState::FocusedSticky;   return;
    default: return;
    }
}

void
Client::unfocus() noexcept
{
    m_focused = false;

    switch (m_outside_state) {
    case OutsideState::Focused:         m_outside_state = OutsideState::Unfocused;         return;
    case OutsideState::FocusedDisowned: m_outside_state = OutsideState::UnfocusedDisowned; return;
    case OutsideState::FocusedSticky:   m_outside_state = OutsideState::UnfocusedSticky;   return;
    default: return;
    }
}

void
Client::stick() noexcept
{
    m_sticky = true;

    switch (m_outside_state) {
    case OutsideState::Focused:   m_outside_state = OutsideState::FocusedSticky;   return;
    case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedSticky; return;
    default: return;
    }
}

void
Client::unstick() noexcept
{
    m_sticky = false;

    switch (m_outside_state) {
    case OutsideState::FocusedSticky:   m_outside_state = OutsideState::Focused;   return;
    case OutsideState::UnfocusedSticky: m_outside_state = OutsideState::Unfocused; return;
    default: return;
    }
}

void
Client::disown() noexcept
{
    m_disowned = true;

    switch (m_outside_state) {
    case OutsideState::Focused:   m_outside_state = OutsideState::FocusedDisowned;   return;
    case OutsideState::Unfocused: m_outside_state = OutsideState::UnfocusedDisowned; return;
    default: return;
    }
}

void
Client::reclaim() noexcept
{
    m_disowned = false;

    switch (m_outside_state) {
    case OutsideState::FocusedDisowned:   m_outside_state = OutsideState::Focused;   return;
    case OutsideState::UnfocusedDisowned: m_outside_state = OutsideState::Unfocused; return;
    default: return;
    }
}

void
Client::set_tile_region(Region& region) noexcept
{
    m_tile_region = region;
    set_active_region(region);
}

void
Client::set_free_region(Region& region) noexcept
{
    m_free_region = region;
    set_active_region(region);
}

void
Client::set_active_region(Region& region) noexcept
{
    m_previous_region = m_active_region;
    set_inner_region(region);
    m_active_region = region;
}

void
Client::set_tile_decoration(Decoration const& decoration) noexcept
{
    m_tile_decoration = decoration;
    m_active_decoration = decoration;
}

void
Client::set_free_decoration(Decoration const& decoration) noexcept
{
    m_free_decoration = decoration;
    m_active_decoration = decoration;
}

void
Client::set_inner_region(Region& region) noexcept
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
