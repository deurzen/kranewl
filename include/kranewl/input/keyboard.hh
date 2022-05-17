#pragma once

extern "C" {
#include <wlr/backend.h>
}

class Server;
struct Keyboard {
    struct wl_list link;
    Server* server;
    struct wlr_input_device* device;

    struct wl_listener modifiers;
    struct wl_listener key;
};
