#pragma once

extern "C" {
#include <wlr/backend.h>
#include <wlr/util/box.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <string>

struct View;
class Server final
{
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
    static void new_xdg_surface(struct wl_listener*, void*);
    static void new_input(struct wl_listener*, void*);
    static void new_pointer(Server*, struct wlr_input_device*);
    static void new_keyboard(Server*, struct wlr_input_device*);

    static void cursor_motion(struct wl_listener*, void*);
    static void cursor_motion_absolute(struct wl_listener*, void*);
    static void cursor_axis(struct wl_listener*, void*);
    static void cursor_button(struct wl_listener*, void*);
    static void cursor_frame(struct wl_listener*, void*);
    static void cursor_process_motion(Server*, uint32_t);
    static void cursor_process_move(Server*, uint32_t);
    static void cursor_process_resize(Server*, uint32_t);

    static void keyboard_handle_modifiers(struct wl_listener*, void*);
    static void keyboard_handle_key(struct wl_listener*, void*);
    static bool keyboard_handle_keybinding(Server*, xkb_keysym_t);

    static void seat_request_cursor(struct wl_listener*, void*);
    static void seat_request_set_selection(struct wl_listener*, void*);

    static void output_frame(struct wl_listener*, void*);
    static View* desktop_view_at(Server*, double, double, struct wlr_surface**, double*, double*);
    static void focus_view(View*, struct wlr_surface*);

    static void xdg_toplevel_map(struct wl_listener*, void*);
    static void xdg_toplevel_unmap(struct wl_listener*, void*);
    static void xdg_toplevel_destroy(struct wl_listener*, void*);
    static void xdg_toplevel_request_move(struct wl_listener*, void*);
    static void xdg_toplevel_request_resize(struct wl_listener*, void*);
    static void xdg_toplevel_handle_moveresize(View*, CursorMode, uint32_t);

    struct wl_display* m_display;
    struct wlr_backend* m_backend;
    struct wlr_renderer* m_renderer;
    struct wlr_allocator* m_allocator;
    struct wlr_scene* m_scene;

    struct wlr_xdg_shell* m_xdg_shell;
    struct wl_listener m_new_xdg_surface;
    struct wl_list m_views;

    struct wlr_cursor* m_cursor;
    struct wlr_xcursor_manager* m_cursor_mgr;
    struct wl_listener m_cursor_motion;
    struct wl_listener m_cursor_motion_absolute;
    struct wl_listener m_cursor_button;
    struct wl_listener m_cursor_axis;
    struct wl_listener m_cursor_frame;

    struct wlr_seat* m_seat;
    struct wl_listener m_new_input;
    struct wl_listener m_request_cursor;
    struct wl_listener m_request_set_selection;
    struct wl_list m_keyboards;
    CursorMode m_cursor_mode;
    View* m_grabbed_view;
    double m_grab_x, m_grab_y;
    struct wlr_box m_grab_geobox;
    uint32_t m_resize_edges;

    struct wlr_output_layout* m_output_layout;
    struct wl_list m_outputs;
    struct wl_listener m_new_output;

    const std::string m_socket;

};
