#pragma once

extern "C" {
#include <wlr/backend.h>
}

class Server;
struct Output {
    struct wl_list link;
    Server* server;
    struct wlr_output* wlr_output;
    struct wl_listener frame;
};
