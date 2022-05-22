#include <trace.hh>

#include <kranewl/tree/view.hh>
#include <kranewl/tree/xdg_view.hh>

extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

XDGView::XDGView(
    struct wlr_xdg_surface* wlr_xdg_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace
)
    : View(
          this,
          reinterpret_cast<std::uintptr_t>(wlr_xdg_surface),
          server,
          model,
          seat,
          output,
          context,
          workspace,
          wlr_xdg_surface->surface,
          XDGView::handle_foreign_activate_request,
          XDGView::handle_foreign_fullscreen_request,
          XDGView::handle_foreign_close_request,
          XDGView::handle_foreign_destroy,
          XDGView::handle_surface_new_subsurface
      ),
      mp_wlr_xdg_surface(wlr_xdg_surface),
      mp_wlr_xdg_toplevel(wlr_xdg_surface->toplevel),
      ml_commit({ .notify = XDGView::handle_commit }),
      ml_request_move({ .notify = XDGView::handle_request_move }),
      ml_request_resize({ .notify = XDGView::handle_request_resize }),
      ml_request_fullscreen({ .notify = XDGView::handle_request_fullscreen }),
      ml_set_title({ .notify = XDGView::handle_set_title }),
      ml_set_app_id({ .notify = XDGView::handle_set_app_id }),
      ml_new_popup({ .notify = XDGView::handle_new_popup }),
      ml_map({ .notify = XDGView::handle_map }),
      ml_unmap({ .notify = XDGView::handle_unmap }),
      ml_destroy({ .notify = XDGView::handle_destroy })
{
    wl_signal_add(&mp_wlr_xdg_surface->events.map, &ml_map);
    wl_signal_add(&mp_wlr_xdg_surface->events.unmap, &ml_unmap);
    wl_signal_add(&mp_wlr_xdg_surface->events.destroy, &ml_destroy);
}

XDGView::~XDGView()
{}

void
XDGView::handle_foreign_activate_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_fullscreen_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_close_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_foreign_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_surface_new_subsurface(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_commit(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_move(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_resize(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_request_fullscreen(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_set_title(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_set_app_id(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_new_popup(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_map(struct wl_listener* listener, void* data)
{
    TRACE();

    XDGView_ptr view = wl_container_of(listener, view, ml_map);
    struct wlr_xdg_toplevel* wlr_xdg_toplevel = view->mp_wlr_xdg_toplevel;

    view->m_mapped = true;
    view->m_preferred_dim = Dim{
        .w = wlr_xdg_toplevel->base->current.geometry.width,
        .h = wlr_xdg_toplevel->base->current.geometry.height,
    };

    if (!view->m_preferred_dim.w && !view->m_preferred_dim.h) {
        view->m_preferred_dim.w = wlr_xdg_toplevel->base->surface->current.width;
        view->m_preferred_dim.h = wlr_xdg_toplevel->base->surface->current.height;
    }

    View::map_view(
        view,
        wlr_xdg_toplevel->base->surface,
        wlr_xdg_toplevel->requested.fullscreen,
        wlr_xdg_toplevel->requested.fullscreen_output,
        false // TODO: determine if client has decorations
    );

    wl_signal_add(&wlr_xdg_toplevel->base->surface->events.commit, &view->ml_commit);
    wl_signal_add(&wlr_xdg_toplevel->base->events.new_popup, &view->ml_new_popup);
    wl_signal_add(&wlr_xdg_toplevel->events.request_fullscreen, &view->ml_request_fullscreen);
    wl_signal_add(&wlr_xdg_toplevel->events.request_move, &view->ml_request_move);
    wl_signal_add(&wlr_xdg_toplevel->events.request_resize, &view->ml_request_resize);
    wl_signal_add(&wlr_xdg_toplevel->events.set_title, &view->ml_set_title);
    wl_signal_add(&wlr_xdg_toplevel->events.set_app_id, &view->ml_set_app_id);
}

void
XDGView::handle_unmap(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XDGView::handle_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

}
