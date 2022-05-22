#pragma once

#include <kranewl/tree/view.hh>

#if XWAYLAND
struct XWaylandView final : public View {
    XWaylandView();
    ~XWaylandView();

    struct wl_listener ml_commit;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
    struct wl_listener ml_request_maximize;
    struct wl_listener ml_request_minimize;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_request_activate;
    struct wl_listener ml_set_title;
    struct wl_listener ml_set_class;
    struct wl_listener ml_set_role;
    struct wl_listener ml_set_window_type;
    struct wl_listener ml_set_hints;
    struct wl_listener ml_set_decorations;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;
    struct wl_listener ml_override_redirect;

};

struct XWaylandUnmanaged final {
    XWaylandUnmanaged();
    ~XWaylandUnmanaged();

    Pos m_pos;

    struct wlr_xwayland_surface* m_wlr_xwayland_surface;

    struct wl_listener ml_request_activate;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_commit;
    struct wl_listener ml_set_geometry;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;
    struct wl_listener ml_override_redirect;

};
#endif
