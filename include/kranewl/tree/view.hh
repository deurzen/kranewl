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
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    int x, y;
};
