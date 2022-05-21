#include <trace.hh>

#include <kranewl/input/seat.hh>

extern "C" {
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_cursor.h>
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
      ml_new_node({ .notify = Seat::handle_new_node }),
      ml_new_input({ .notify = Seat::handle_new_input }),
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

    /* wl_signal_add(&seat->events.__, &ml_new_node); */
    /* wl_signal_add(&seat->events.new_input, &ml_new_input); */
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

void
Seat::handle_new_node(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_new_input(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_cursor_motion(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_cursor_motion_absolute(struct wl_listener*, void*)
{
    TRACE();

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
Seat::handle_cursor_frame(struct wl_listener*, void*)
{
    TRACE();

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
