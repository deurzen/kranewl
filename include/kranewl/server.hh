#pragma once

#include <kranewl/geometry.hh>
#include <kranewl/input/seat.hh>

extern "C" {
#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <string>

typedef class Model* Model_ptr;
typedef class View* View_ptr;
typedef class Server* Server_ptr;

typedef class Server final {
    enum class CursorMode {
        Passthrough,
        Move,
        Resize,
    };

public:
    Server(Model_ptr);
    ~Server();

    void run() noexcept;

    void moveresize_view(View_ptr, Region const&, Extents const&, bool);

private:
    static void handle_new_output(struct wl_listener*, void*);
    static void handle_output_layout_change(struct wl_listener*, void*);
    static void handle_output_manager_apply(struct wl_listener*, void*);
    static void handle_output_manager_test(struct wl_listener*, void*);
    static void handle_new_xdg_surface(struct wl_listener*, void*);
    static void handle_new_layer_shell_surface(struct wl_listener*, void*);
    static void handle_xdg_activation(struct wl_listener*, void*);
    static void handle_new_input(struct wl_listener*, void*);
    static void handle_inhibit_activate(struct wl_listener*, void*);
    static void handle_inhibit_deactivate(struct wl_listener*, void*);
    static void handle_idle_inhibitor_create(struct wl_listener*, void*);
    static void handle_idle_inhibitor_destroy(struct wl_listener*, void*);
    static void handle_xdg_new_toplevel_decoration(struct wl_listener*, void*);
    static void handle_xdg_toplevel_map(struct wl_listener*, void*);
    static void handle_xdg_toplevel_unmap(struct wl_listener*, void*);
    static void handle_xdg_toplevel_destroy(struct wl_listener*, void*);
    static void handle_xdg_toplevel_request_move(struct wl_listener*, void*);
    static void handle_xdg_toplevel_request_resize(struct wl_listener*, void*);
    static void handle_xdg_toplevel_handle_moveresize(View_ptr, CursorMode, uint32_t);
#ifdef XWAYLAND
    static void handle_xwayland_ready(struct wl_listener*, void*);
    static void handle_new_xwayland_surface(struct wl_listener*, void*);
    static void handle_xwayland_request_activate(struct wl_listener*, void*);
    static void handle_xwayland_request_configure(struct wl_listener*, void*);
    static void handle_xwayland_set_hints(struct wl_listener*, void*);
#endif

    Model_ptr mp_model;

public:
    struct wl_display* mp_display;
    struct wl_event_loop* mp_event_loop;

    struct wlr_backend* mp_backend;
    struct wlr_renderer* mp_renderer;
    struct wlr_allocator* mp_allocator;
    struct wlr_compositor* mp_compositor;
    struct wlr_data_device_manager* mp_data_device_manager;
    struct wlr_output_layout* mp_output_layout;
    struct wlr_scene* mp_scene;
    struct wlr_scene_node* m_layers[7];
#ifdef XWAYLAND
    struct wlr_xwayland* mp_xwayland;
#endif

    Seat m_seat;


private:
    struct wlr_xdg_shell* mp_xdg_shell;
    struct wlr_layer_shell_v1* mp_layer_shell;
    struct wlr_xdg_activation_v1* mp_xdg_activation;
    struct wlr_output_manager_v1* mp_output_manager;
    struct wlr_presentation* mp_presentation;
    struct wlr_idle* mp_idle;
    struct wlr_server_decoration_manager* mp_server_decoration_manager;
    struct wlr_xdg_decoration_manager_v1* mp_xdg_decoration_manager;
    struct wlr_pointer_constraints_v1* mp_pointer_constraints;
    struct wlr_relative_pointer_manager_v1* mp_relative_pointer_manager;
    struct wlr_virtual_pointer_manager_v1* mp_virtual_pointer_manager;
    struct wlr_virtual_keyboard_manager_v1* mp_virtual_keyboard_manager;
    struct wlr_input_inhibit_manager* mp_input_inhibit_manager;
    struct wlr_idle_inhibit_manager_v1* mp_idle_inhibit_manager;
    struct wlr_keyboard_shortcuts_inhibit_manager_v1* mp_keyboard_shortcuts_inhibit_manager;

    struct wl_listener ml_new_output;
    struct wl_listener ml_output_layout_change;
    struct wl_listener ml_output_manager_apply;
    struct wl_listener ml_output_manager_test;
    struct wl_listener ml_new_xdg_surface;
    struct wl_listener ml_new_layer_shell_surface;
    struct wl_listener ml_xdg_activation;
    struct wl_listener ml_new_input;
    struct wl_listener ml_inhibit_activate;
    struct wl_listener ml_inhibit_deactivate;
    struct wl_listener ml_idle_inhibitor_create;
    struct wl_listener ml_idle_inhibitor_destroy;
    struct wl_listener ml_xdg_new_toplevel_decoration;
#ifdef XWAYLAND
    struct wl_listener ml_xwayland_ready;
    struct wl_listener ml_new_xwayland_surface;
#endif

    const std::string m_socket;

}* Server_ptr;
