#include <trace.hh>

#include <kranewl/layers.hh>
#include <kranewl/server.hh>
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
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace,
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
      mp_output(output),
      mp_context(context),
      mp_workspace(workspace),
      mp_wlr_surface(wlr_surface),
      m_alpha(1.f),
      m_tile_decoration(FREE_DECORATION),
      m_free_decoration(FREE_DECORATION),
      m_active_decoration(FREE_DECORATION),
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
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace,
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
      mp_output(output),
      mp_context(context),
      mp_workspace(workspace),
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
        }
        break;
#if HAVE_XWAYLAND
    case View::Type::XWayland:
        {
            struct wlr_xwayland_surface* wlr_xwayland_surface
                = wlr_xwayland_surface_from_wlr_surface(view->mp_wlr_surface);
            view->m_pid = wlr_xwayland_surface->pid;
        }
        break;
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

    if (fullscreen_output && fullscreen_output->data) {
        Output_ptr output = reinterpret_cast<Output_ptr>(fullscreen_output->data);
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
