#pragma once

#include <kranewl/geometry.hh>

extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/backend.h>
#include <wlr/util/edges.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <unordered_set>

struct CursorInput {
    enum class Target
    {
        Global,
        Root,
        View
    };

    enum Button : uint32_t {
        Left        = BTN_LEFT,
        Right       = BTN_RIGHT,
        Middle      = BTN_MIDDLE,
        Forward     = BTN_SIDE,
        Backward    = BTN_EXTRA,
        ScrollUp    = KEY_MAX + 1,
        ScrollDown  = KEY_MAX + 2,
        ScrollLeft  = KEY_MAX + 3,
        ScrollRight = KEY_MAX + 4,
    };

    Target target;
	Button button;
	uint32_t modifiers;
};

typedef class Server* Server_ptr;
typedef class Seat* Seat_ptr;
typedef struct View* View_ptr;
typedef struct Node* Node_ptr;

typedef struct Cursor {
    enum class Mode {
        Passthrough,
        Move,
        Resize,
    };

    Cursor(
        Server_ptr,
        Seat_ptr,
        struct wlr_cursor*,
        struct wlr_pointer_constraints_v1*,
        struct wlr_relative_pointer_manager_v1*,
        struct wlr_virtual_pointer_manager_v1*
    );
    ~Cursor();

    View_ptr view_under_cursor() const;
    Node_ptr node_under_cursor() const;

    void initiate_cursor_interactive(Mode, View_ptr, uint32_t);
    void initiate_cursor_interactive(Mode, View_ptr);
    void abort_cursor_interactive();

    void process_cursor_motion(uint32_t time);

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

    struct wlr_cursor* mp_wlr_cursor;
    struct wlr_xcursor_manager* mp_cursor_manager;
    struct wlr_pointer_constraints_v1* mp_pointer_constraints;
    struct wlr_relative_pointer_manager_v1* mp_relative_pointer_manager;
    struct wlr_virtual_pointer_manager_v1* mp_virtual_pointer_manager;

    Mode m_cursor_mode;
    struct {
        View_ptr view;
        double x, y;
        Region region;
        uint32_t edges;
    } m_grab_state;

    struct wl_listener ml_cursor_motion;
    struct wl_listener ml_cursor_motion_absolute;
    struct wl_listener ml_cursor_button;
    struct wl_listener ml_cursor_axis;
    struct wl_listener ml_cursor_frame;
    struct wl_listener ml_request_start_drag;
    struct wl_listener ml_start_drag;
    struct wl_listener ml_request_set_cursor;

}* Cursor_ptr;

inline bool
operator==(CursorInput const& lhs, CursorInput const& rhs)
{
    return lhs.target == rhs.target
        && lhs.button == rhs.button
        && lhs.modifiers == rhs.modifiers;
}

namespace std
{
    template <>
    struct hash<CursorInput> {
        std::size_t
        operator()(CursorInput const& input) const
        {
            std::size_t target_hash = std::hash<CursorInput::Target>()(input.target);
            std::size_t modifiers_hash = std::hash<uint32_t>()(input.modifiers);
            std::size_t button_hash = std::hash<uint32_t>()(input.button);

            return target_hash ^ modifiers_hash ^ button_hash;
        }
    };
}
