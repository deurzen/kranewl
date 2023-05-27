#pragma once

#include <kranewl/geometry.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/xdg-decoration.hh>
#include <kranewl/xwayland.hh>

extern "C" {
#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
}

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

typedef class Model* Model_ptr;
typedef class Server* Server_ptr;
typedef struct View* View_ptr;
typedef struct XDGDecoration* XDGDecoration_ptr;

typedef class Server final {
    static constexpr int XDG_SHELL_VERSION = 2;
    static constexpr int LAYER_SHELL_VERSION = 3;

public:
    Server(Model_ptr);
    ~Server();

    void initialize();
    void start();
    void run();
    void terminate();

    void relinquish_focus();

private:
    static void handle_new_output(struct wl_listener*, void*);
    static void handle_output_layout_change(struct wl_listener*, void*);
    static void handle_output_manager_apply_or_test(Server_ptr, struct wlr_output_configuration_v1*, bool);
    static void handle_output_manager_apply(struct wl_listener*, void*);
    static void handle_output_manager_test(struct wl_listener*, void*);
    static void handle_new_xdg_surface(struct wl_listener*, void*);
    static void handle_new_layer_shell_surface(struct wl_listener*, void*);
    static void handle_new_input(struct wl_listener*, void*);
    static void handle_new_xdg_toplevel_decoration(struct wl_listener*, void*);
    static void handle_xdg_request_activate(struct wl_listener*, void*);
    static void handle_new_virtual_keyboard(struct wl_listener*, void*);
    static void handle_drm_lease_request(struct wl_listener*, void*);

    static void propagate_output_layout_change(Server_ptr);
    static void configure_libinput(struct wlr_input_device*);

    Model_ptr mp_model = nullptr;

public:
    struct wl_display* mp_display;
    struct wl_event_loop* mp_event_loop;

    struct wlr_backend* mp_backend;
    struct wlr_backend* mp_headless_backend;
    struct wlr_renderer* mp_renderer;
    struct wlr_allocator* mp_allocator;
    struct wlr_compositor* mp_compositor;
#ifdef XWAYLAND
    struct wlr_xwayland* mp_wlr_xwayland;
    XWayland_ptr mp_xwayland;
#endif
    struct wlr_data_device_manager* mp_data_device_manager;
    struct wlr_output_layout* mp_output_layout;
    struct wlr_scene* mp_scene;
    std::array<struct wlr_scene_tree*, 8> m_scene_layers;
    Seat_ptr mp_seat;
    struct wlr_output* mp_fallback_output;
    struct wlr_output_manager_v1* mp_output_manager;
    struct wlr_drm_lease_v1_manager* mp_drm_lease_manager;

    std::unordered_map<Uid, XDGDecoration_ptr> m_decorations;

private:
    struct wlr_xdg_shell* mp_xdg_shell;
    struct wlr_layer_shell_v1* mp_layer_shell;
    struct wlr_xdg_activation_v1* mp_xdg_activation;
    struct wlr_presentation* mp_presentation;
    struct wlr_server_decoration_manager* mp_server_decoration_manager;
    struct wlr_xdg_decoration_manager_v1* mp_xdg_decoration_manager;
    struct wlr_virtual_keyboard_manager_v1* mp_virtual_keyboard_manager;
    struct wlr_xdg_foreign_registry* mp_foreign_registry;

    struct wl_listener ml_new_output;
    struct wl_listener ml_output_layout_change;
    struct wl_listener ml_output_manager_apply;
    struct wl_listener ml_output_manager_test;
    struct wl_listener ml_new_xdg_surface;
    struct wl_listener ml_new_layer_shell_surface;
    struct wl_listener ml_new_input;
    struct wl_listener ml_new_xdg_toplevel_decoration;
    struct wl_listener ml_xdg_request_activate;
    struct wl_listener ml_new_virtual_keyboard;
    struct wl_listener ml_drm_lease_request;

    std::string m_socket;

}* Server_ptr;
