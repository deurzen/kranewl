#ifdef XWAYLAND
#include <trace.hh>

#include <kranewl/tree/view.hh>
#include <kranewl/tree/xwayland_view.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/xwayland.h>
}
#undef static
#undef class
#undef namespace

XWaylandView::XWaylandView(
    struct wlr_xwayland_surface* wlr_xwayland_surface,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat
)
    : View(
          this,
          reinterpret_cast<std::uintptr_t>(wlr_xwayland_surface),
          server,
          model,
          seat,
          wlr_xwayland_surface->surface
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

Region
XWaylandView::constraints()
{
    TRACE();

}

pid_t
XWaylandView::pid()
{
    TRACE();

    struct wlr_xwayland_surface* wlr_xwayland_surface
        = wlr_xwayland_surface_from_wlr_surface(mp_wlr_surface);
    return wlr_xwayland_surface->pid;
}

bool
XWaylandView::prefers_floating()
{
    TRACE();

}

View_ptr
XWaylandView::is_transient_for()
{
    TRACE();

}

void
XWaylandView::map()
{
    TRACE();

}

void
XWaylandView::unmap()
{
    TRACE();

}

void
XWaylandView::focus(Toggle)
{
    TRACE();

}

void
XWaylandView::activate(Toggle)
{
    TRACE();

}

void
XWaylandView::set_tiled(Toggle)
{
    TRACE();

}

void
XWaylandView::set_fullscreen(Toggle)
{
    TRACE();

}

void
XWaylandView::set_resizing(Toggle)
{
    TRACE();

}

void
XWaylandView::configure(Region const& region, Extents const& extents, bool interactive)
{
    TRACE();

}

void
XWaylandView::close()
{
    TRACE();

}

void
XWaylandView::close_popups()
{
    TRACE();

}

void
XWaylandView::destroy()
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

XWaylandUnmanaged::XWaylandUnmanaged(struct wlr_xwayland_surface* wlr_xwayland_surface)
    : mp_wlr_xwayland_surface(wlr_xwayland_surface)
{}

XWaylandUnmanaged::~XWaylandUnmanaged()
{}
#endif
