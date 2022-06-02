#include <trace.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/util.hh>

extern "C" {
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>
}

Keyboard::Keyboard(Server_ptr server, Seat_ptr seat, struct wlr_input_device* device)
    : mp_server(server),
      mp_seat(seat),
      mp_device(device),
      ml_destroy({ .notify = handle_destroy }),
      ml_modifiers({ .notify = handle_modifiers }),
      ml_key({ .notify = handle_key })
{
    wl_signal_add(&mp_device->keyboard->events.modifiers, &ml_modifiers);
    wl_signal_add(&mp_device->keyboard->events.key, &ml_key);
    wl_signal_add(&mp_device->events.destroy, &ml_destroy);
}

Keyboard::~Keyboard()
{}

void
Keyboard::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Keyboard::handle_modifiers(struct wl_listener* listener, void*)
{
    TRACE();

    Keyboard_ptr keyboard = wl_container_of(listener, keyboard, ml_modifiers);
    Seat_ptr seat = keyboard->mp_seat;

    wlr_seat_set_keyboard(seat->mp_wlr_seat, keyboard->mp_device);
    wlr_seat_keyboard_notify_modifiers(
        seat->mp_wlr_seat,
        &keyboard->mp_device->keyboard->modifiers
    );
}

static inline bool
process_key_binding(Model_ptr model, KeyboardInput input)
{
    TRACE();

    auto binding = Util::const_retrieve(model->key_bindings(), input);

    if (binding) {
        (*binding)(*model);
        return true;
    }

    return false;
}

void
Keyboard::handle_key(struct wl_listener* listener, void* data)
{
    TRACE();

    Keyboard_ptr keyboard = wl_container_of(listener, keyboard, ml_key);
    Seat_ptr seat = keyboard->mp_seat;

    struct wlr_event_keyboard_key* event
        = reinterpret_cast<struct wlr_event_keyboard_key*>(data);

    uint32_t keycode = event->keycode + 8;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->mp_device->keyboard);

    const xkb_keysym_t* keysyms;
    int symcount = xkb_state_key_get_syms(
        keyboard->mp_device->keyboard->xkb_state,
        keycode,
        &keysyms
    );

    wlr_idle_notify_activity(
        seat->mp_idle,
        seat->mp_wlr_seat
    );

    bool key_press_handled = false;
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < symcount; ++i)
            if (keysyms[i] >= XKB_KEY_XF86Switch_VT_1 &&
                    keysyms[i] <= XKB_KEY_XF86Switch_VT_12)
            {
                struct wlr_session* session
                    = wlr_backend_get_session(seat->mp_server->mp_backend);

                if (session) {
                    unsigned vt = keysyms[i] - XKB_KEY_XF86Switch_VT_1 + 1;
                    spdlog::info("Switching to VT {}", vt);
                    wlr_session_change_vt(session, vt);
                }

                return;
            }

        if (!seat->mp_input_inhibit_manager->active_inhibitor) {
            for (int i = 0; i < symcount; ++i)
                key_press_handled |= process_key_binding(
                    seat->mp_model,
                    KeyboardInput{
                        keysyms[i],
                        modifiers & ~WLR_MODIFIER_CAPS
                    }
                );
        }
    }

	if (!key_press_handled) {
		wlr_seat_set_keyboard(seat->mp_wlr_seat, keyboard->mp_device);
		wlr_seat_keyboard_notify_key(
            seat->mp_wlr_seat,
            event->time_msec,
			event->keycode,
            event->state
        );
	}
}
