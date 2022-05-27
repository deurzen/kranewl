#pragma once

extern "C" {
#include <wlr/backend.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <unordered_set>

struct KeyboardInput {
	xkb_keysym_t keysym;
	uint32_t modifiers;
};

typedef class Server* Server_ptr;
typedef class Seat* Seat_ptr;

typedef struct Keyboard {
    Keyboard(Server_ptr, Seat_ptr, struct wlr_input_device*);
    ~Keyboard();

    static void handle_destroy(struct wl_listener*, void*);
    static void handle_modifiers(struct wl_listener*, void*);
    static void handle_key(struct wl_listener*, void*);

    Server_ptr mp_server;
	Seat_ptr mp_seat;

	struct wlr_input_device* mp_device;

    struct wl_listener ml_destroy;
    struct wl_listener ml_modifiers;
    struct wl_listener ml_key;

}* Keyboard_ptr;

inline bool
operator==(KeyboardInput const& lhs, KeyboardInput const& rhs)
{
    return lhs.keysym == rhs.keysym
        && lhs.modifiers == rhs.modifiers;
}

namespace std
{
    template <>
    struct hash<KeyboardInput> {
        std::size_t
        operator()(KeyboardInput const& input) const
        {
            std::size_t key_hash = std::hash<xkb_keysym_t>()(input.keysym);
            std::size_t modifiers_hash = std::hash<uint32_t>()(input.modifiers);

            return modifiers_hash ^ key_hash;
        }
    };
}
