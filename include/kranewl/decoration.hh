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
    RGBA nextfocus;
    RGBA prevfocus;
};

const static ColorScheme DEFAULT_COLOR_SCHEME = ColorScheme{
    .focused   = RGBA{0xB294BBFF},
    .fdisowned = RGBA{0xB294BBFF},
    .fsticky   = RGBA{0xB294BBFF},
    .unfocused = RGBA{0x1D1F21FF},
    .udisowned = RGBA{0x1D1F21FF},
    .usticky   = RGBA{0x1D1F21FF},
    .urgent    = RGBA{0xCC6666FF},
    .nextfocus = RGBA{0xCC6666FF},
    .prevfocus = RGBA{0xCC6666FF},
};

const static ColorScheme DEFAULT_FREE_COLOR_SCHEME = ColorScheme{
    .focused   = RGBA{0xB294BBFF},
    .fdisowned = RGBA{0xB294BBFF},
    .fsticky   = RGBA{0xB294BBFF},
    .unfocused = RGBA{0x373B41FF},
    .udisowned = RGBA{0x373B41FF},
    .usticky   = RGBA{0x373B41FF},
    .urgent    = RGBA{0xCC6666FF},
    .nextfocus = RGBA{0xCC6666FF},
    .prevfocus = RGBA{0xCC6666FF},
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
        Extents{4, 3, 2, 2}
    },
    DEFAULT_FREE_COLOR_SCHEME
};

const int CYCLE_INDICATOR_SIZE = 15;
