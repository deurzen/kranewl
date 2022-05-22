#include <trace.hh>

#include <kranewl/tree/view.hh>
#include <kranewl/tree/xwayland_view.hh>

#if XWAYLAND
XWaylandView::XWaylandView(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat,
    Output_ptr output,
    Context_ptr context,
    Workspace_ptr workspace
)
    : View(
          this,
          reinterpret_cast<std::uintptr_t>(wlr_xwayland_surface),
          server,
          model,
          seat,
          output,
          context,
          workspace,
          XWaylandView::handle_foreign_activate_request,
          XWaylandView::handle_foreign_fullscreen_request,
          XWaylandView::handle_foreign_close_request,
          XWaylandView::handle_foreign_destroy,
          XWaylandView::handle_surface_new_subsurface
      ),
      mp_wlr_xwayland_surface(wlr_xwayland_surface),
      ml_commit({ .notify = XWaylandView::handle_commit }),
      ml_request_move({ .notify = XWaylandView::handle_request_move }),
      ml_request_resize({ .notify = XWaylandView::handle_request_resize }),
      ml_request_maximize({ .notify = XWaylandView::handle_request_maximize }),
      ml_request_minimize({ .notify = XWaylandView::handle_request_minimize }),
      ml_request_configure({ .notify = XWaylandView::handle_request_configure }),
      ml_request_fullscreen({ .notify = XWaylandView::handle_request_fullscreen }),
      ml_request_activate({ .notify = XWaylandView::handle_request_activate }),
      ml_set_title({ .notify = XWaylandView::handle_set_title }),
      ml_set_class({ .notify = XWaylandView::handle_set_class }),
      ml_set_role({ .notify = XWaylandView::handle_set_role }),
      ml_set_window_type({ .notify = XWaylandView::handle_set_window_type }),
      ml_set_hints({ .notify = XWaylandView::handle_set_hints }),
      ml_set_decorations({ .notify = XWaylandView::handle_set_decorations }),
      ml_map({ .notify = XWaylandView::handle_map }),
      ml_unmap({ .notify = XWaylandView::handle_unmap }),
      ml_destroy({ .notify = XWaylandView::handle_destroy }),
      ml_override_redirect({ .notify = XWaylandView::handle_override_redirect })
{}

XWaylandView::~XWaylandView()
{}

XWaylandUnmanaged::XWaylandUnmanaged(struct wlr_xwayland_surface* wlr_xwayland_surface)
    : mp_wlr_xwayland_surface(wlr_xwayland_surface)
{}

XWaylandUnmanaged::~XWaylandUnmanaged()
{}

void
XWaylandView::handle_foreign_activate_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_foreign_fullscreen_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_foreign_close_request(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_foreign_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_surface_new_subsurface(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_commit(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_move(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_resize(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_maximize(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_minimize(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_configure(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_fullscreen(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_request_activate(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_title(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_class(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_role(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_window_type(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_hints(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_set_decorations(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_map(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_unmap(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_destroy(struct wl_listener* listener, void* data)
{
    TRACE();

}

void
XWaylandView::handle_override_redirect(struct wl_listener* listener, void* data)
{
    TRACE();

}
#endif
