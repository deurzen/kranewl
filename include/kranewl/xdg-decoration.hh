#pragma once

#include <kranewl/common.hh>

extern "C" {
#include <wayland-server-core.h>
}

#include <unordered_map>
#include <vector>

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef struct XDGView* XDGView_ptr;

typedef struct XDGDecoration final {
public:
    XDGDecoration(
        Server_ptr,
        Manager_ptr,
        XDGView_ptr,
        struct wlr_xdg_toplevel_decoration_v1*
    );

    ~XDGDecoration();

    static void handle_request_mode(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

public:
    Uid m_uid;

    Server_ptr mp_server;
    Manager_ptr mp_manager;
    XDGView_ptr mp_view;

    struct wlr_xdg_toplevel_decoration_v1* mp_wlr_xdg_decoration;

    struct wl_listener ml_request_mode;
    struct wl_listener ml_destroy;

}* XDGDecoration_ptr;
