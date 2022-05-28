#pragma once

#include <kranewl/geometry.hh>

extern "C" {
#include <wlr/backend.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <unordered_set>

struct MouseInput {
	unsigned button;
	uint32_t modifiers;
};

typedef class Server* Server_ptr;
typedef class Seat* Seat_ptr;
typedef struct View* View_ptr;

typedef struct Mouse {
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

    Mouse(
        Server_ptr,
        Seat_ptr,
        struct wlr_cursor*,
        struct wlr_pointer_constraints_v1*,
        struct wlr_relative_pointer_manager_v1*,
        struct wlr_virtual_pointer_manager_v1*
    );
    ~Mouse();

    View_ptr view_under_cursor() const;

    static void handle_cursor_motion(struct wl_listener*, void*);
    static void handle_cursor_motion_absolute(struct wl_listener*, void*);
    static void handle_cursor_button(struct wl_listener*, void*);
    static void handle_cursor_axis(struct wl_listener*, void*);
    static void handle_cursor_frame(struct wl_listener*, void*);
    static void handle_request_start_drag(struct wl_listener*, void*);
    static void handle_start_drag(struct wl_listener*, void*);
    static void handle_request_set_cursor(struct wl_listener*, void*);

    Server_ptr mp_server;
	Seat_ptr mp_seat;

    CursorMode m_cursor_mode;
    struct wlr_cursor* mp_cursor;
    struct wlr_xcursor_manager* mp_cursor_manager;
    struct wlr_pointer_constraints_v1* mp_pointer_constraints;
    struct wlr_relative_pointer_manager_v1* mp_relative_pointer_manager;
    struct wlr_virtual_pointer_manager_v1* mp_virtual_pointer_manager;

    struct {
        View_ptr client;
        double x, y;
        Region region;
        uint32_t resize_edges;
    } m_grab_state;

    struct wl_listener ml_cursor_motion;
    struct wl_listener ml_cursor_motion_absolute;
    struct wl_listener ml_cursor_button;
    struct wl_listener ml_cursor_axis;
    struct wl_listener ml_cursor_frame;
    struct wl_listener ml_request_start_drag;
    struct wl_listener ml_start_drag;
    struct wl_listener ml_request_set_cursor;

}* Mouse_ptr;

inline bool
operator==(MouseInput const& lhs, MouseInput const& rhs)
{
    return lhs.button == rhs.button
        && lhs.modifiers == rhs.modifiers;
}

namespace std
{
    template <>
    struct hash<MouseInput> {
        std::size_t
        operator()(MouseInput const& input) const
        {
            std::size_t modifiers_hash = std::hash<uint32_t>()(input.modifiers);
            std::size_t button_hash = std::hash<unsigned>()(input.button);

            return modifiers_hash ^ button_hash;
        }
    };
}
