#include <trace.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/util.hh>

extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
}

Seat::Seat(
    Server_ptr server,
    Model_ptr model,
    struct wlr_seat* seat,
    struct wlr_cursor* cursor
)
    : mp_server(server),
      mp_model(model),
      mp_seat(seat),
      mp_cursor(cursor),
      mp_cursor_manager(wlr_xcursor_manager_create(NULL, 24)),
      ml_destroy({ .notify = Seat::handle_destroy }),
      ml_cursor_motion({ .notify = Seat::handle_cursor_motion }),
      ml_cursor_motion_absolute({ .notify = Seat::handle_cursor_motion_absolute }),
      ml_cursor_button({ .notify = Seat::handle_cursor_button }),
      ml_cursor_axis({ .notify = Seat::handle_cursor_axis }),
      ml_cursor_frame({ .notify = Seat::handle_cursor_frame }),
      ml_request_start_drag({ .notify = Seat::handle_request_start_drag }),
      ml_start_drag({ .notify = Seat::handle_start_drag }),
      ml_request_set_cursor({ .notify = Seat::handle_request_set_cursor }),
      ml_request_set_selection({ .notify = Seat::handle_request_set_selection }),
      ml_request_set_primary_selection({ .notify = Seat::handle_request_set_primary_selection })
{
    TRACE();

    wlr_xcursor_manager_load(mp_cursor_manager, 1);

    wl_signal_add(&seat->events.destroy, &ml_destroy);
    wl_signal_add(&cursor->events.motion, &ml_cursor_motion);
    wl_signal_add(&cursor->events.motion_absolute, &ml_cursor_motion_absolute);
    wl_signal_add(&cursor->events.button, &ml_cursor_button);
    wl_signal_add(&cursor->events.axis, &ml_cursor_axis);
    wl_signal_add(&cursor->events.frame, &ml_cursor_frame);
    wl_signal_add(&seat->events.request_start_drag, &ml_request_start_drag);
    wl_signal_add(&seat->events.start_drag, &ml_start_drag);
    wl_signal_add(&seat->events.request_set_cursor, &ml_request_set_cursor);
    wl_signal_add(&seat->events.request_set_selection, &ml_request_set_selection);
    wl_signal_add(&seat->events.request_set_primary_selection, &ml_request_set_primary_selection);
}

Seat::~Seat()
{
    TRACE();

}

static inline void
process_cursor_move(Seat_ptr seat, uint32_t time)
{

}

static inline void
process_cursor_resize(Seat_ptr seat, uint32_t time)
{

}

static inline void
process_cursor_motion(Seat_ptr seat, uint32_t time)
{
    switch (seat->m_cursor_mode) {
    case Seat::CursorMode::Move:   process_cursor_move(seat, time);   return;
    case Seat::CursorMode::Resize: process_cursor_resize(seat, time); return;
    case Seat::CursorMode::Passthrough: // fallthrough
    default: break;
    }

    double sx, sy;
    struct wlr_surface* surface = NULL;
    // TODO: get client under cursor

    if (true /* no client under cursor? */) {
        wlr_xcursor_manager_set_cursor_image(
            seat->mp_cursor_manager,
            "left_ptr",
            seat->mp_cursor
        );
    }

    if (surface) {
        wlr_seat_pointer_notify_enter(seat->mp_seat, surface, sx, sy);
        wlr_seat_pointer_notify_motion(seat->mp_seat, time, sx, sy);
    } else
        wlr_seat_pointer_clear_focus(seat->mp_seat);
}

Keyboard_ptr
Seat::create_keyboard(struct wlr_input_device* device)
{
    Keyboard_ptr keyboard = new Keyboard(mp_server, this, device);
    register_keyboard(keyboard);
    return keyboard;
}

void
Seat::register_keyboard(Keyboard_ptr keyboard)
{
    m_keyboards.push_back(keyboard);
}

void
Seat::unregister_keyboard(Keyboard_ptr keyboard)
{
    Util::erase_remove(m_keyboards, keyboard);
}

void
Seat::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_cursor_motion(struct wl_listener* listener, void* data)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_cursor_motion);
    struct wlr_event_pointer_motion* event
        = reinterpret_cast<struct wlr_event_pointer_motion*>(data);

    wlr_cursor_move(seat->mp_cursor, event->device, event->delta_x, event->delta_y);
    process_cursor_motion(seat, event->time_msec);
}

void
Seat::handle_cursor_motion_absolute(struct wl_listener* listener, void* data)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute* event
        = reinterpret_cast<struct wlr_event_pointer_motion_absolute*>(data);

    wlr_cursor_warp_absolute(seat->mp_cursor, event->device, event->x, event->y);
    process_cursor_motion(seat, event->time_msec);
}

void
Seat::handle_cursor_button(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_cursor_axis(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_cursor_frame(struct wl_listener* listener, void*)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_cursor_frame);
    wlr_seat_pointer_notify_frame(seat->mp_seat);
}

void
Seat::handle_request_start_drag(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_start_drag(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_request_set_cursor(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_request_set_selection(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_request_set_primary_selection(struct wl_listener*, void*)
{
    TRACE();

}
