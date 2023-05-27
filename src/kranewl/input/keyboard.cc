#include <trace.hh>

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/seat.hh>
#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/util.hh>

extern "C" {
#include <wayland-server-core.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_input_inhibitor.h>
#include <wlr/types/wlr_keyboard.h>
#include <xkbcommon/xkbcommon.h>
}

Keyboard::Keyboard(Server_ptr server, Seat_ptr seat, struct wlr_keyboard* wlr_keyboard)
    : mp_server(server),
      mp_seat(seat),
      mp_wlr_keyboard(wlr_keyboard),
      ml_modifiers({ .notify = handle_modifiers }),
      ml_key({ .notify = handle_key })
      /* ml_destroy({ .notify = handle_destroy }) */
{
    mp_key_repeat_source = wl_event_loop_add_timer(
        server->mp_event_loop,
        handle_key_repeat,
        this
    );

    wl_signal_add(&mp_wlr_keyboard->events.modifiers, &ml_modifiers);
    wl_signal_add(&mp_wlr_keyboard->events.key, &ml_key);
    /* wl_signal_add(&mp_device->events.destroy, &ml_destroy); */
}

Keyboard::~Keyboard()
{}

void
Keyboard::handle_modifiers(struct wl_listener* listener, void*)
{
    TRACE();

    Keyboard_ptr keyboard = wl_container_of(listener, keyboard, ml_modifiers);
    Seat_ptr seat = keyboard->mp_seat;

    wlr_seat_set_keyboard(seat->mp_wlr_seat, keyboard->mp_wlr_keyboard);
    wlr_seat_keyboard_notify_modifiers(
        seat->mp_wlr_seat,
        &keyboard->mp_wlr_keyboard->modifiers
    );
}

static inline void
perform_action(Model_ptr model, std::function<void(Model&)> action)
{
    action(*model);
}

static inline std::optional<KeyboardAction>
process_key_binding(Model_ptr model, KeyboardInput input)
{
    TRACE();

    auto binding = Util::const_retrieve(model->key_bindings(), input);

    if (binding) {
        perform_action(model, binding->action);
        return binding;
    }

    return std::nullopt;
}

static inline void
disarm_key_repeat_timer(Keyboard_ptr keyboard)
{
    if (!keyboard)
        return;

    keyboard->m_repeat_action = std::nullopt;
    if (wl_event_source_timer_update(keyboard->mp_key_repeat_source, 0) < 0)
        spdlog::error("Could not disarm key repeat timer");
}

void
Keyboard::handle_key(struct wl_listener* listener, void* data)
{
    TRACE();

    Keyboard_ptr keyboard = wl_container_of(listener, keyboard, ml_key);
    Seat_ptr seat = keyboard->mp_seat;

    struct wlr_keyboard_key_event* event
        = reinterpret_cast<struct wlr_keyboard_key_event*>(data);

    uint32_t keycode = event->keycode + 8;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->mp_wlr_keyboard);

    const xkb_keysym_t* keysyms;
    int symcount = xkb_state_key_get_syms(
        keyboard->mp_wlr_keyboard->xkb_state,
        keycode,
        &keysyms
    );

    wlr_idle_notify_activity(
        seat->mp_idle,
        seat->mp_wlr_seat
    );

    bool key_press_handled = false;
    switch (event->state) {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
    {
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

        if (!seat->mp_input_inhibit_manager->active_inhibitor)
            for (int i = 0; i < symcount; ++i) {
                std::optional<KeyboardAction> binding = process_key_binding(
                    seat->mp_model,
                    KeyboardInput{
                        keysyms[i],
                        modifiers & ~WLR_MODIFIER_CAPS
                    }
                );

                if (binding && binding->repeatable
                    && keyboard->mp_wlr_keyboard->repeat_info.delay > 0)
                {
                    keyboard->m_repeat_action = binding->action;
                    if (wl_event_source_timer_update(keyboard->mp_key_repeat_source,
                        keyboard->mp_wlr_keyboard->repeat_info.delay) < 0)
                    {
                        spdlog::error("Could not set up key repeat timer");
                    }
                } else if (keyboard->m_repeat_action)
                    disarm_key_repeat_timer(keyboard);

                key_press_handled |= binding != std::nullopt;
            }

        break;
    }
    case WL_KEYBOARD_KEY_STATE_RELEASED:
        disarm_key_repeat_timer(keyboard);
        break;
    default: break;
    }

	if (!key_press_handled) {
		wlr_seat_set_keyboard(seat->mp_wlr_seat, keyboard->mp_wlr_keyboard);
		wlr_seat_keyboard_notify_key(
            seat->mp_wlr_seat,
            event->time_msec,
			event->keycode,
            event->state
        );
	}
}

int
Keyboard::handle_key_repeat(void* data)
{
    Keyboard_ptr keyboard = reinterpret_cast<Keyboard_ptr>(data);
    struct wlr_keyboard* wlr_device = keyboard->mp_wlr_keyboard;

    if (keyboard->m_repeat_action) {
        if (wlr_device->repeat_info.rate > 0)
            if (wl_event_source_timer_update(keyboard->mp_key_repeat_source,
                1000 / wlr_device->repeat_info.rate) < 0)
            {
                spdlog::error("Could not update key repeat timer");
            }

        perform_action(
            keyboard->mp_seat->mp_model,
            *keyboard->m_repeat_action
        );
    }

    return 0;
}

/* void */
/* Keyboard::handle_destroy(struct wl_listener* listener, void*) */
/* { */
/*     TRACE(); */

/*     Keyboard_ptr keyboard = wl_container_of(listener, keyboard, ml_destroy); */
/*     Seat_ptr seat = keyboard->mp_seat; */

/*     if (wlr_seat_get_keyboard(seat->mp_wlr_seat) == keyboard->mp_device->keyboard) */
/*         keyboard->mp_device->keyboard = nullptr; */

/* 	wl_list_remove(&keyboard->ml_key.link); */
/* 	wl_list_remove(&keyboard->ml_modifiers.link); */

/*     disarm_key_repeat_timer(keyboard); */
/*     wl_event_source_remove(keyboard->mp_key_repeat_source); */

/*     seat->unregister_keyboard(keyboard); */
/* } */
