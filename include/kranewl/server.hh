#pragma once

#include <kranewl/geometry.hh>

extern "C" {
#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <string>

typedef struct Client* Client_ptr;
typedef class Server* Server_ptr;
typedef class Server final {
    enum class CursorMode {
        Passthrough,
        Move,
        Resize,
    };

public:
    Server();
    ~Server();

    void start() noexcept;

private:
    static void new_output(struct wl_listener*, void*);
    static void output_layout_change(struct wl_listener*, void*);
    static void output_manager_apply(struct wl_listener*, void*);
    static void output_manager_test(struct wl_listener*, void*);

    static void new_xdg_surface(struct wl_listener*, void*);
    static void new_layer_shell_surface(struct wl_listener*, void*);
    static void xdg_activation(struct wl_listener*, void*);

    static void new_input(struct wl_listener*, void*);
    static void new_pointer(Server*, struct wlr_input_device*);
    static void new_keyboard(Server*, struct wlr_input_device*);
    static void inhibit_activate(struct wl_listener*, void*);
    static void inhibit_deactivate(struct wl_listener*, void*);
    static void idle_inhibitor_create(struct wl_listener*, void*);
    static void idle_inhibitor_destroy(struct wl_listener*, void*);

    static void cursor_motion(struct wl_listener*, void*);
    static void cursor_motion_absolute(struct wl_listener*, void*);
    static void cursor_axis(struct wl_listener*, void*);
    static void cursor_button(struct wl_listener*, void*);
    static void cursor_frame(struct wl_listener*, void*);
    static void request_set_cursor(struct wl_listener*, void*);
    static void cursor_process_motion(Server*, uint32_t);
    static void cursor_process_move(Server*, uint32_t);
    static void cursor_process_resize(Server*, uint32_t);
    static void request_start_drag(struct wl_listener*, void*);
    static void start_drag(struct wl_listener*, void*);

    static void keyboard_handle_modifiers(struct wl_listener*, void*);
    static void keyboard_handle_key(struct wl_listener*, void*);
    static bool keyboard_handle_keybinding(Server*, xkb_keysym_t);
    static void request_set_selection(struct wl_listener*, void*);
    static void request_set_primary_selection(struct wl_listener*, void*);

    static void output_frame(struct wl_listener*, void*);
    static Client_ptr desktop_client_at(Server_ptr, double, double, struct wlr_surface**, double*, double*);
    static void focus_client(Client_ptr, struct wlr_surface*);

    static void xdg_toplevel_map(struct wl_listener*, void*);
    static void xdg_toplevel_unmap(struct wl_listener*, void*);
    static void xdg_toplevel_destroy(struct wl_listener*, void*);
    static void xdg_toplevel_request_move(struct wl_listener*, void*);
    static void xdg_toplevel_request_resize(struct wl_listener*, void*);
    static void xdg_toplevel_handle_moveresize(Client_ptr, CursorMode, uint32_t);

#ifdef XWAYLAND
    static void xwayland_ready(struct wl_listener*, void*);
    static void new_xwayland_surface(struct wl_listener*, void*);
    static void xwayland_request_activate(struct wl_listener*, void*);
    static void xwayland_request_configure(struct wl_listener*, void*);
    static void xwayland_set_hints(struct wl_listener*, void*);
#endif

    struct wl_display* m_display;
    struct wl_event_loop* m_event_loop;

    struct wlr_backend* m_backend;
    struct wlr_renderer* m_renderer;
    struct wlr_allocator* m_allocator;
    struct wlr_compositor* m_compositor;
    struct wlr_data_device_manager* m_data_device_manager;

#ifdef XWAYLAND
    struct wlr_xwayland* m_xwayland;
#endif

    struct wlr_output_layout* m_output_layout;
    struct wlr_scene* m_scene;
    struct wlr_xdg_shell* m_xdg_shell;
    struct wlr_layer_shell_v1* m_layer_shell;
    struct wlr_xdg_activation_v1* m_xdg_activation;

    struct wlr_output_manager_v1* m_output_manager;
    struct wlr_presentation* m_presentation;
    struct wlr_idle* m_idle;

    struct wlr_server_decoration_manager* m_server_decoration_manager;
    struct wlr_xdg_decoration_manager_v1* m_xdg_decoration_manager;

    struct wlr_seat* m_seat;
    struct wlr_cursor* m_cursor;
    struct wlr_xcursor_manager* m_cursor_manager;
    struct wlr_pointer_constraints_v1* m_pointer_constraints;
    struct wlr_relative_pointer_manager_v1* m_relative_pointer_manager;
    struct wlr_virtual_pointer_manager_v1* m_virtual_pointer_manager;
    struct wlr_virtual_keyboard_manager_v1* m_virtual_keyboard_manager;
    struct wlr_input_inhibit_manager* m_input_inhibit_manager;
    struct wlr_idle_inhibit_manager_v1* m_idle_inhibit_manager;
    struct wlr_keyboard_shortcuts_inhibit_manager_v1* m_keyboard_shortcuts_inhibit_manager;

    struct wl_list m_outputs;
    struct wl_list m_clients;
    struct wl_list m_keyboards;

    struct wl_listener ml_new_output;
    struct wl_listener ml_output_layout_change;
    struct wl_listener ml_output_manager_apply;
    struct wl_listener ml_output_manager_test;
    struct wl_listener ml_new_xdg_surface;
    struct wl_listener ml_new_layer_shell_surface;
    struct wl_listener ml_xdg_activation;
    struct wl_listener ml_cursor_motion;
    struct wl_listener ml_cursor_motion_absolute;
    struct wl_listener ml_cursor_button;
    struct wl_listener ml_cursor_axis;
    struct wl_listener ml_cursor_frame;
    struct wl_listener ml_request_set_cursor;
    struct wl_listener ml_request_start_drag;
    struct wl_listener ml_start_drag;
    struct wl_listener ml_new_input;
    struct wl_listener ml_request_set_selection;
    struct wl_listener ml_request_set_primary_selection;
    struct wl_listener ml_inhibit_activate;
    struct wl_listener ml_inhibit_deactivate;
    struct wl_listener ml_idle_inhibitor_create;
    struct wl_listener ml_idle_inhibitor_destroy;
#ifdef XWAYLAND
    struct wl_listener ml_xwayland_ready;
    struct wl_listener ml_new_xwayland_surface;
#endif

    CursorMode m_cursor_mode;
    Client_ptr m_grabbed_client;
    double m_grab_x, m_grab_y;
    struct wlr_box m_grab_geobox;
    uint32_t m_resize_edges;

    const std::string m_socket;

}* Server_ptr;
