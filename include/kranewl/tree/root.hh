#pragma once

#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
#include <kranewl/tree/node.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/backend.h>
}

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;

typedef class Root final : public Node {
public:
    Root(Server_ptr, Model_ptr, struct wlr_output_layout*, Output_ptr);
    ~Root();

    static void handle_output_layout_change(struct wl_listener*, void*);

public:
    Server_ptr mp_server;
    Model_ptr mp_model;

    struct wlr_output_layout* mp_output_layout;

    struct wl_listener ml_output_layout_change;

    struct {
        struct wl_signal new_node;
    } m_events;

    Region m_region;

    Cycle<Output_ptr> m_outputs;
    Cycle<Container_ptr> m_scratchpad;

    Output_ptr mp_fallback_output;
    Container_ptr mp_fullscreen_global;

}* Root_ptr;
