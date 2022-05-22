#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
}

#include <vector>

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Keyboard* Keyboard_ptr;
typedef class Client* Client_ptr;

typedef class Seat final {
public:
    enum class CursorMode {
        Passthrough,
        Move,
        Resize,
    };

    enum class CursorButton {
        Left   = 272,
        Right  = 273,
        Middle = 274,
    };

    Seat(Server_ptr, Model_ptr, struct wlr_seat*, struct wlr_cursor*);
    ~Seat();

    static void handle_new_node(struct wl_listener*, void*);
    static void handle_new_input(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);
    static void handle_cursor_motion(struct wl_listener*, void*);
    static void handle_cursor_motion_absolute(struct wl_listener*, void*);
    static void handle_cursor_button(struct wl_listener*, void*);
    static void handle_cursor_axis(struct wl_listener*, void*);
    static void handle_cursor_frame(struct wl_listener*, void*);
    static void handle_request_start_drag(struct wl_listener*, void*);
    static void handle_start_drag(struct wl_listener*, void*);
    static void handle_request_set_cursor(struct wl_listener*, void*);
    static void handle_request_set_selection(struct wl_listener*, void*);
    static void handle_request_set_primary_selection(struct wl_listener*, void*);

public:
    Server_ptr mp_server;
    Model_ptr mp_model;

    struct wlr_seat* mp_seat;
    struct wlr_cursor* mp_cursor;
    CursorMode m_cursor_mode;

    std::vector<Keyboard_ptr> m_keyboards;

    struct {
        Client_ptr client;
        double x, y;
        Region region;
        uint32_t resize_edges;
    } m_grab_state;

    struct wl_client* mp_exclusive_client;

    struct wl_listener ml_new_node;
    struct wl_listener ml_new_input;
    struct wl_listener ml_destroy;
    struct wl_listener ml_cursor_motion;
    struct wl_listener ml_cursor_motion_absolute;
    struct wl_listener ml_cursor_button;
    struct wl_listener ml_cursor_axis;
    struct wl_listener ml_cursor_frame;
    struct wl_listener ml_request_start_drag;
    struct wl_listener ml_start_drag;
    struct wl_listener ml_request_set_cursor;
    struct wl_listener ml_request_set_selection;
    struct wl_listener ml_request_set_primary_selection;

}* Seat_ptr;