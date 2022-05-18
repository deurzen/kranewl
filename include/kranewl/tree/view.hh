#pragma once

extern "C" {
#include <wlr/backend.h>
}

class Server;
struct View {
    struct wl_list link;

    Server* server;

    struct wlr_xdg_surface* xdg_surface;
    struct wlr_scene_node* scene_node;

    struct wl_listener l_map;
    struct wl_listener l_unmap;
    struct wl_listener l_destroy;
    struct wl_listener l_request_move;
    struct wl_listener l_request_resize;

    int x, y;
};
