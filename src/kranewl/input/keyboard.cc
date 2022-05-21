#include <trace.hh>

#include <kranewl/input/keyboard.hh>

Keyboard::Keyboard(Server_ptr server, Seat_ptr seat, struct wlr_input_device* device)
    : mp_server(server),
      mp_seat(seat),
      mp_device(device),
      ml_destroy({ .notify = handle_destroy }),
      ml_modifiers({ .notify = handle_modifiers }),
      ml_key({ .notify = handle_key })
{
}

Keyboard::~Keyboard()
{}

void
Keyboard::handle_destroy(struct wl_listener*, void*)
{
    TRACE();

}

void
Keyboard::handle_modifiers(struct wl_listener*, void*)
{
    TRACE();

}

void
Keyboard::handle_key(struct wl_listener*, void*)
{
    TRACE();

}
