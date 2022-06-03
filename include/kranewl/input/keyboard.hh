#pragma once

extern "C" {
#include <wlr/backend.h>
#include <xkbcommon/xkbcommon.h>
}

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_set>

struct KeyboardInput {
    xkb_keysym_t keysym;
    uint32_t modifiers;
};

class Model;
struct KeyboardAction {
    std::function<void(Model&)> action;
    bool repeatable;
};

typedef class Server* Server_ptr;
typedef class Seat* Seat_ptr;

typedef struct Keyboard {
    Keyboard(Server_ptr, Seat_ptr, struct wlr_input_device*);
    ~Keyboard();

    static void handle_modifiers(struct wl_listener*, void*);
    static void handle_key(struct wl_listener*, void*);
    static int handle_key_repeat(void*);
    static void handle_destroy(struct wl_listener*, void*);

    Server_ptr mp_server;
    Seat_ptr mp_seat;

    struct wlr_input_device* mp_device;

    uint32_t m_repeat_rate;
    uint32_t m_repeat_delay;

    struct wl_event_source* mp_key_repeat_source;
    std::optional<std::function<void(Model&)>>  m_repeat_action;

    struct wl_listener ml_modifiers;
    struct wl_listener ml_key;
    struct wl_listener ml_destroy;

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
