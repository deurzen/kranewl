#pragma once

#include <cstdlib>

enum Layer : short {
    None       = -1,
    Background = 0,
    Bottom     = 1,
    Tile       = 2,
    Free       = 3,
    Top        = 4,
    Overlay    = 5,
    NoFocus    = 6,
};
