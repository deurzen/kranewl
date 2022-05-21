#pragma once

extern "C" {
#include <wlr/backend.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <unordered_set>

typedef class Server* Server_ptr;

typedef struct Keyboard {
    Server_ptr mp_server;

    struct wl_list m_link;
	struct wlr_input_device* mp_device;

    struct wl_listener ml_modifiers;
    struct wl_listener ml_key;
}* Keyboard_ptr;

struct KeyboardInput {
	uint32_t mod;
	xkb_keysym_t keysym;
};

namespace std
{
    template <>
    struct hash<KeyboardInput> {
        std::size_t
        operator()(KeyboardInput const& input) const
        {
            std::size_t mod_hash = std::hash<uint32_t>()(input.mod);
            std::size_t key_hash = std::hash<xkb_keysym_t>()(input.keysym);

            return mod_hash ^ key_hash;
        }
    };
}
