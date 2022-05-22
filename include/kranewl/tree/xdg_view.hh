#pragma once

#include <kranewl/tree/view.hh>

struct XDGView final : public View {
    XDGView();
    ~XDGView();

    struct wl_listener ml_commit;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_set_title;
    struct wl_listener ml_set_app_id;
    struct wl_listener ml_new_popup;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;

};
