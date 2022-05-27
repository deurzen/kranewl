#pragma once

#include <kranewl/geometry.hh>

#include <optional>

struct RGBA final {
    RGBA(unsigned hex_)
        : hex(hex_)
    {
        values[3] = static_cast<float>(hex & 0xff)         / 255.f;
        values[2] = static_cast<float>((hex >> 8) & 0xff)  / 255.f;
        values[1] = static_cast<float>((hex >> 16) & 0xff) / 255.f;
        values[0] = static_cast<float>((hex >> 24) & 0xff) / 255.f;
    }

    RGBA(const float values_[4])
        : hex(((static_cast<unsigned>(values_[0] * 255.f) & 0xff) << 24) +
              ((static_cast<unsigned>(values_[1] * 255.f) & 0xff) << 16) +
              ((static_cast<unsigned>(values_[2] * 255.f) & 0xff) << 8) +
              ((static_cast<unsigned>(values_[3] * 255.f) & 0xff)))
    {
        values[0] = values_[0];
        values[1] = values_[1];
        values[2] = values_[2];
        values[3] = values_[3];
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
    .focused   = RGBA{0x8181A6FF},
    .fdisowned = RGBA{0xc1c1c1FF},
    .fsticky   = RGBA{0x5F8787FF},
    .unfocused = RGBA{0x333333FF},
    .udisowned = RGBA{0x999999FF},
    .usticky   = RGBA{0x444444FF},
    .urgent    = RGBA{0x87875FFF},
};

struct Frame final {
    Extents extents;
};

struct Decoration final {
    std::optional<Frame> frame;
    ColorScheme colorscheme;

    Extents const& extents() const
    {
        static Extents NO_EXTENTS{0, 0, 0, 0};
        if (frame)
            return frame->extents;

        return NO_EXTENTS;
    }
};

const Decoration NO_DECORATION = Decoration{
    std::nullopt,
    DEFAULT_COLOR_SCHEME
};

const Decoration FREE_DECORATION = Decoration{
    Frame{
        Extents{3, 1, 1, 1}
    },
    DEFAULT_COLOR_SCHEME
};
