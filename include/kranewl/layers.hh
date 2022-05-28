#pragma once

struct Layer {
    typedef short type;
    static constexpr short None = -1;
    static constexpr short Background = 0;
    static constexpr short Bottom     = 1;
    static constexpr short Tile       = 2;
    static constexpr short Free       = 3;
    static constexpr short Top        = 4;
    static constexpr short Overlay    = 5;
    static constexpr short NoFocus    = 6;
};
