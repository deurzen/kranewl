#pragma once

#include <kranewl/common.hh>
#include <kranewl/geometry.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_output.h>
}

#include <vector>

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef struct Keyboard* Keyboard_ptr;
typedef struct Cursor* Cursor_ptr;

typedef class Seat final {
public:
    Seat(
        Server_ptr,
        Manager_ptr,
        struct wlr_seat*,
        struct wlr_idle*,
        struct wlr_cursor*,
        struct wlr_input_inhibit_manager*,
        struct wlr_idle_inhibit_manager_v1*,
        struct wlr_virtual_keyboard_manager_v1*
    );
    ~Seat();

    Keyboard_ptr create_keyboard(struct wlr_keyboard*);
    void register_keyboard(Keyboard_ptr);
    void unregister_keyboard(Keyboard_ptr);

    static void handle_destroy(struct wl_listener*, void*);
    static void handle_request_set_selection(struct wl_listener*, void*);
    static void handle_request_set_primary_selection(struct wl_listener*, void*);
    static void handle_idle_new_inhibitor(struct wl_listener*, void*);
    static void handle_idle_destroy_inhibitor(struct wl_listener*, void*);
    static void handle_input_inhibit_activate(struct wl_listener*, void*);
    static void handle_input_inhibit_deactivate(struct wl_listener*, void*);

public:
    Server_ptr mp_server;
    Manager_ptr mp_manager;

    struct wlr_seat* mp_wlr_seat;
    struct wlr_idle* mp_idle;
    struct wlr_input_inhibit_manager* mp_input_inhibit_manager;
    struct wlr_idle_inhibit_manager_v1* mp_idle_inhibit_manager;
    struct wlr_virtual_keyboard_manager_v1* mp_virtual_keyboard_manager;

    Cursor_ptr mp_cursor;
    std::vector<Keyboard_ptr> m_keyboards;

    struct wl_listener ml_destroy;
    struct wl_listener ml_request_set_selection;
    struct wl_listener ml_request_set_primary_selection;
    struct wl_listener ml_idle_new_inhibitor;
    struct wl_listener ml_idle_destroy_inhibitor;
    struct wl_listener ml_input_inhibit_activate;
    struct wl_listener ml_input_inhibit_deactivate;

}* Seat_ptr;
