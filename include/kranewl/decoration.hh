#pragma once

#include <kranewl/geometry.hh>

#include <optional>

struct RGBA final {
    RGBA(unsigned _hex)
        : hex(_hex)
    {
        values[0] = static_cast<float>(hex & 0xff)         / 255.f;
        values[1] = static_cast<float>((hex >> 8) & 0xff)  / 255.f;
        values[2] = static_cast<float>((hex >> 16) & 0xff) / 255.f;
        values[3] = static_cast<float>((hex >> 24) & 0xff) / 255.f;
    }

    RGBA(const float _values[4])
        : hex(((static_cast<unsigned>(_values[3] * 255.f) & 0xff) << 24) +
              ((static_cast<unsigned>(_values[2] * 255.f) & 0xff) << 16) +
              ((static_cast<unsigned>(_values[1] * 255.f) & 0xff) << 8) +
              ((static_cast<unsigned>(_values[0] * 255.f) & 0xff)))
    {
        values[0] = _values[0];
        values[1] = _values[1];
        values[2] = _values[2];
        values[3] = _values[3];
    }

    unsigned hex;
    float values[4];
};

struct ColorScheme final {
    RGBA focused;
    RGBA fdisowned;
    RGBA fsticky;
    RGBA unfocused;
    RGBA udisowned;
    RGBA usticky;
    RGBA urgent;
};

const static ColorScheme DEFAULT_COLOR_SCHEME = ColorScheme{
    .focused   = {0x8181A6FF},
    .fdisowned = {0xc1c1c1FF},
    .fsticky   = {0x5F8787FF},
    .unfocused = {0x333333FF},
    .udisowned = {0x999999FF},
    .usticky   = {0x444444FF},
    .urgent    = {0x87875FFF},
};

struct Frame final {
    Extents extents;
    ColorScheme colorscheme;
};

struct Decoration final {
    std::optional<Frame> frame;

    const Extents extents() const
    {
        if (frame)
            return frame->extents;

        return Extents{0, 0, 0, 0};
    }
};

const Decoration NO_DECORATION = Decoration{
    std::nullopt,
};

const Decoration FREE_DECORATION = Decoration{
    Frame {
        Extents{ 3, 1, 1, 1 },
        DEFAULT_COLOR_SCHEME
    }
};
