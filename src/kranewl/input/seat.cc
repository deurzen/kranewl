#include <trace.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/cursor.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/util.hh>

extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
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
    struct wlr_pointer_constraints_v1* pointer_constraints,
    struct wlr_relative_pointer_manager_v1* relative_pointer_manager,
    struct wlr_virtual_pointer_manager_v1* virtual_pointer_manager,
    struct wlr_virtual_keyboard_manager_v1* virtual_keyboard_manager,
    struct wlr_keyboard_shortcuts_inhibit_manager_v1* keyboard_shortcuts_inhibit_manager
)
    : mp_server(server),
      mp_model(model),
      mp_wlr_seat(seat),
      mp_idle(idle),
      mp_input_inhibit_manager(input_inhibit_manager),
      mp_idle_inhibit_manager(idle_inhibit_manager),
      mp_virtual_keyboard_manager(virtual_keyboard_manager),
      mp_keyboard_shortcuts_inhibit_manager(keyboard_shortcuts_inhibit_manager),
      mp_cursor(new Cursor(
          server,
          model,
          this,
          cursor,
          pointer_constraints,
          relative_pointer_manager,
          virtual_pointer_manager
      )),
      m_keyboards(),
      ml_destroy({ .notify = Seat::handle_destroy }),
      ml_request_set_selection({ .notify = Seat::handle_request_set_selection }),
      ml_request_set_primary_selection({ .notify = Seat::handle_request_set_primary_selection }),
      ml_inhibit_manager_new_inhibitor({ .notify = Seat::handle_inhibit_manager_new_inhibitor }),
      ml_inhibit_manager_inhibit_activate({ .notify = Seat::handle_inhibit_manager_inhibit_activate }),
      ml_inhibit_manager_inhibit_deactivate({ .notify = Seat::handle_inhibit_manager_inhibit_deactivate })
{
    TRACE();

    wl_signal_add(&seat->events.destroy, &ml_destroy);
    wl_signal_add(&seat->events.request_set_selection, &ml_request_set_selection);
    wl_signal_add(&seat->events.request_set_primary_selection, &ml_request_set_primary_selection);
    wl_signal_add(&mp_idle_inhibit_manager->events.new_inhibitor, &ml_inhibit_manager_new_inhibitor);
    wl_signal_add(&mp_input_inhibit_manager->events.activate, &ml_inhibit_manager_inhibit_activate);
    wl_signal_add(&mp_input_inhibit_manager->events.deactivate, &ml_inhibit_manager_inhibit_deactivate);
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
Seat::handle_inhibit_manager_new_inhibitor(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_inhibit_manager_inhibit_activate(struct wl_listener*, void*)
{
    TRACE();

}

void
Seat::handle_inhibit_manager_inhibit_deactivate(struct wl_listener*, void*)
{
    TRACE();

}
