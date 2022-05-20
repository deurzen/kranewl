#pragma once

#include <cstdint>
#include <unordered_set>

struct MouseInput {
	uint32_t mod;
	unsigned button;
};

namespace std
{
    template <>
    struct hash<MouseInput> {
        std::size_t
        operator()(MouseInput const& input) const
        {
            std::size_t mod_hash = std::hash<uint32_t>()(input.mod);
            std::size_t button_hash = std::hash<unsigned>()(input.button);

            return mod_hash ^ button_hash;
        }
    };
}
