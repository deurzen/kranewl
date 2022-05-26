#include <trace.hh>

#include <kranewl/layers.hh>
#include <kranewl/server.hh>
#include <kranewl/model.hh>
#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg_view.hh>

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
#undef class
#undef namespace

View::View(
    XDGView_ptr,
    Uid uid,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    struct wlr_surface* wlr_surface,
    void(*handle_foreign_activate_request)(wl_listener*, void*),
    void(*handle_foreign_fullscreen_request)(wl_listener*, void*),
    void(*handle_foreign_close_request)(wl_listener*, void*),
    void(*handle_foreign_destroy)(wl_listener*, void*),
    void(*handle_surface_new_subsurface)(wl_listener*, void*)
)
    : m_uid(uid),
      m_type(Type::XDGShell),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_output(nullptr),
      mp_context(nullptr),
      mp_workspace(nullptr),
      mp_wlr_surface(wlr_surface),
      m_alpha(1.f),
      m_tile_decoration(FREE_DECORATION),
      m_free_decoration(FREE_DECORATION),
      m_active_decoration(FREE_DECORATION),
      m_minimum_dim({}),
      m_preferred_dim({}),
      m_free_region({}),
      m_tile_region({}),
      m_active_region({}),
      m_previous_region({}),
      m_inner_region({}),
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
      m_last_focused(std::chrono::steady_clock::now()),
      m_managed_since(std::chrono::steady_clock::now()),
      ml_foreign_activate_request({ .notify = handle_foreign_activate_request }),
      ml_foreign_fullscreen_request({ .notify = handle_foreign_fullscreen_request }),
      ml_foreign_close_request({ .notify = handle_foreign_close_request }),
      ml_foreign_destroy({ .notify = handle_foreign_destroy }),
      ml_surface_new_subsurface({ .notify = handle_surface_new_subsurface })
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
    struct wlr_surface* wlr_surface,
    void(*handle_foreign_activate_request)(wl_listener*, void*),
    void(*handle_foreign_fullscreen_request)(wl_listener*, void*),
    void(*handle_foreign_close_request)(wl_listener*, void*),
    void(*handle_foreign_destroy)(wl_listener*, void*),
    void(*handle_surface_new_subsurface)(wl_listener*, void*)
)
    : m_uid(uid),
      m_type(Type::XWayland),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_wlr_surface(wlr_surface),
      ml_foreign_activate_request({ .notify = handle_foreign_activate_request }),
      ml_foreign_fullscreen_request({ .notify = handle_foreign_fullscreen_request }),
      ml_foreign_close_request({ .notify = handle_foreign_close_request }),
      ml_foreign_destroy({ .notify = handle_foreign_destroy }),
      ml_surface_new_subsurface({ .notify = handle_surface_new_subsurface })
{}
#endif

View::~View()
{}

static inline void
set_view_pid(View_ptr view)
{
    switch (view->m_type) {
    case View::Type::XDGShell:
    {
        pid_t pid;
        struct wl_client* client
            = wl_resource_get_client(view->mp_wlr_surface->resource);

        wl_client_get_credentials(client, &pid, NULL, NULL);
        view->m_pid = pid;

        break;
    }
#if HAVE_XWAYLAND
    case View::Type::XWayland:
    {
        struct wlr_xwayland_surface* wlr_xwayland_surface
            = wlr_xwayland_surface_from_wlr_surface(view->mp_wlr_surface);
        view->m_pid = wlr_xwayland_surface->pid;

        break;
    }
#endif
    default: break;
    }
}

void
View::map_view(
    View_ptr view,
    struct wlr_surface* wlr_surface,
    bool fullscreen,
    struct wlr_output* fullscreen_output,
    bool decorations
)
{
    TRACE();

    Server_ptr server = view->mp_server;
    Model_ptr model = view->mp_model;

    view->mp_wlr_surface = wlr_surface;
    set_view_pid(view);

    view->mp_scene = &wlr_scene_tree_create(view->mp_server->m_layers[Layer::Tile])->node;
    view->mp_wlr_surface->data = view->mp_scene_surface = view->m_type == View::Type::XDGShell
        ? wlr_scene_xdg_surface_create(
            view->mp_scene,
            reinterpret_cast<XDGView_ptr>(view)->mp_wlr_xdg_surface
        )
        : wlr_scene_subsurface_tree_create(view->mp_scene, view->mp_wlr_surface);
    view->mp_scene_surface->data = view;

    wlr_scene_node_reparent(
        view->mp_scene,
        server->m_layers[view->m_floating ? Layer::Free : Layer::Tile]
    );

    // TODO: globalize
    static const float border_rgba[4] = {0.5, 0.5, 0.5, 1.0};

    for (std::size_t i = 0; i < 4; ++i) {
        view->m_protrusions[i] = wlr_scene_rect_create(view->mp_scene, 0, 0, border_rgba);
        view->m_protrusions[i]->node.data = view;
        wlr_scene_rect_set_color(view->m_protrusions[i], border_rgba);
        wlr_scene_node_lower_to_bottom(&view->m_protrusions[i]->node);
    }

    if (fullscreen_output && fullscreen_output->data) {
        Output_ptr output = reinterpret_cast<Output_ptr>(fullscreen_output->data);
    }

    model->move_view_to_focused_output(view);
}


void
View::unmap_view(View_ptr view)
{
    TRACE();

    wlr_scene_node_destroy(view->mp_scene);
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
View::set_active_region(Region const& region)
{
    m_previous_region = m_active_region;
    set_inner_region(region);
    m_active_region = region;
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

ViewChild::ViewChild(SubsurfaceViewChild_ptr)
    : m_type(Type::Subsurface)
{}

ViewChild::ViewChild(PopupViewChild_ptr)
    : m_type(Type::Popup)
{}

ViewChild::~ViewChild()
{}
