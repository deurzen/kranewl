#pragma once

#include <kranewl/geometry.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/layer.hh>

extern "C" {
#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
}

#include <array>
#include <cstdint>
#include <string>

typedef class Model* Model_ptr;
typedef class Server* Server_ptr;
typedef struct View* View_ptr;

typedef class Server final {
public:
    Server(Model_ptr);
    ~Server();

    void run() noexcept;
    void terminate() noexcept;

    void relinquish_focus();

private:
    static void handle_new_output(struct wl_listener*, void*);
    static void handle_output_layout_change(struct wl_listener*, void*);
    static void handle_output_manager_apply(struct wl_listener*, void*);
    static void handle_output_manager_test(struct wl_listener*, void*);
    static void handle_new_xdg_surface(struct wl_listener*, void*);
    static void handle_new_layer_shell_surface(struct wl_listener*, void*);
    static void handle_xdg_activation(struct wl_listener*, void*);
    static void handle_new_input(struct wl_listener*, void*);
    static void handle_xdg_new_toplevel_decoration(struct wl_listener*, void*);
    static void handle_xdg_toplevel_map(struct wl_listener*, void*);
    static void handle_xdg_toplevel_unmap(struct wl_listener*, void*);
    static void handle_xdg_toplevel_destroy(struct wl_listener*, void*);
    static void handle_xdg_toplevel_request_move(struct wl_listener*, void*);
    static void handle_xdg_toplevel_request_resize(struct wl_listener*, void*);
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
#ifdef XWAYLAND
    struct wlr_xwayland* mp_xwayland;
#endif
    struct wlr_data_device_manager* mp_data_device_manager;
    struct wlr_output_layout* mp_output_layout;
    struct wlr_scene* mp_scene;
    std::array<struct wlr_scene_node*, 7> m_layers;
    Seat m_seat;

private:
    struct wlr_xdg_shell* mp_xdg_shell;
    struct wlr_layer_shell_v1* mp_layer_shell;
    struct wlr_xdg_activation_v1* mp_xdg_activation;
    struct wlr_output_manager_v1* mp_output_manager;
    struct wlr_presentation* mp_presentation;
    struct wlr_server_decoration_manager* mp_server_decoration_manager;
    struct wlr_xdg_decoration_manager_v1* mp_xdg_decoration_manager;

    struct wl_listener ml_new_output;
    struct wl_listener ml_output_layout_change;
    struct wl_listener ml_output_manager_apply;
    struct wl_listener ml_output_manager_test;
    struct wl_listener ml_new_xdg_surface;
    struct wl_listener ml_new_layer_shell_surface;
    struct wl_listener ml_xdg_activation;
    struct wl_listener ml_new_input;
    struct wl_listener ml_xdg_new_toplevel_decoration;
#ifdef XWAYLAND
    struct wl_listener ml_xwayland_ready;
    struct wl_listener ml_new_xwayland_surface;
#endif

    const std::string m_socket;

}* Server_ptr;
