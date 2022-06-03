#include <trace.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/cursor.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/util.hh>

extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_primary_selection.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
}

Seat::Seat(
    Server_ptr server,
    Model_ptr model,
    struct wlr_seat* seat,
    struct wlr_idle* idle,
    struct wlr_cursor* cursor,
    struct wlr_input_inhibit_manager* input_inhibit_manager,
    struct wlr_idle_inhibit_manager_v1* idle_inhibit_manager,
    struct wlr_virtual_keyboard_manager_v1* virtual_keyboard_manager
)
    : mp_server(server),
      mp_model(model),
      mp_wlr_seat(seat),
      mp_idle(idle),
      mp_input_inhibit_manager(input_inhibit_manager),
      mp_idle_inhibit_manager(idle_inhibit_manager),
      mp_virtual_keyboard_manager(virtual_keyboard_manager),
      mp_cursor(new Cursor(
          server,
          model,
          this,
          cursor
      )),
      m_keyboards(),
      ml_destroy({ .notify = Seat::handle_destroy }),
      ml_request_set_selection({ .notify = Seat::handle_request_set_selection }),
      ml_request_set_primary_selection({ .notify = Seat::handle_request_set_primary_selection }),
      ml_idle_new_inhibitor({ .notify = Seat::handle_idle_new_inhibitor }),
      ml_idle_destroy_inhibitor({ .notify = Seat::handle_idle_destroy_inhibitor }),
      ml_input_inhibit_activate({ .notify = Seat::handle_input_inhibit_activate }),
      ml_input_inhibit_deactivate({ .notify = Seat::handle_input_inhibit_deactivate })
{
    TRACE();

    wl_signal_add(&seat->events.destroy, &ml_destroy);
    wl_signal_add(&seat->events.request_set_selection, &ml_request_set_selection);
    wl_signal_add(&seat->events.request_set_primary_selection, &ml_request_set_primary_selection);
    wl_signal_add(&mp_idle_inhibit_manager->events.new_inhibitor, &ml_idle_new_inhibitor);
    wl_signal_add(&mp_input_inhibit_manager->events.activate, &ml_input_inhibit_activate);
    wl_signal_add(&mp_input_inhibit_manager->events.deactivate, &ml_input_inhibit_deactivate);
}

Seat::~Seat()
{
    TRACE();

    delete mp_cursor;
    for (Keyboard_ptr keyboard : m_keyboards)
        delete keyboard;

    m_keyboards.clear();
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
    delete keyboard;
}

void
Seat::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_request_set_selection(struct wl_listener* listener, void* data)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_request_set_selection);
    struct wlr_seat_request_set_selection_event* event
        = reinterpret_cast<struct wlr_seat_request_set_selection_event*>(data);

    wlr_seat_set_selection(
        seat->mp_wlr_seat,
        event->source,
        event->serial
    );
}

void
Seat::handle_request_set_primary_selection(struct wl_listener* listener, void* data)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_request_set_primary_selection);
    struct wlr_seat_request_set_primary_selection_event* event
        = reinterpret_cast<struct wlr_seat_request_set_primary_selection_event*>(data);

    wlr_seat_set_primary_selection(
        seat->mp_wlr_seat,
        event->source,
        event->serial
    );
}

void
Seat::handle_idle_new_inhibitor(struct wl_listener* listener, void* data)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_idle_new_inhibitor);
    struct wlr_idle_inhibitor_v1* idle_inhibitor
        = reinterpret_cast<struct wlr_idle_inhibitor_v1*>(data);

    wl_signal_add(&idle_inhibitor->events.destroy, &seat->ml_idle_destroy_inhibitor);
    wlr_idle_set_enabled(seat->mp_idle, seat->mp_wlr_seat, false);
}

void
Seat::handle_idle_destroy_inhibitor(struct wl_listener* listener, void*)
{
    TRACE();

    Seat_ptr seat = wl_container_of(listener, seat, ml_idle_destroy_inhibitor);

    wlr_idle_set_enabled(
        seat->mp_idle,
        seat->mp_wlr_seat,
        wl_list_length(&seat->mp_idle_inhibit_manager->inhibitors) <= 1
    );
}

void
Seat::handle_input_inhibit_activate(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_input_inhibit_deactivate(struct wl_listener*, void*)
{
    TRACE();

}
