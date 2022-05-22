#pragma once

#include <cstddef>
#include <cstdint>

typedef std::uintptr_t Uid;
typedef std::size_t Ident;
typedef std::size_t Index;

enum class Direction {
    Forward,
    Backward,
};

enum class Edge {
    Left,
    Right,
    Top,
    Bottom,
};

enum class Corner {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
};

enum class Toggle {
    On,
    Off,
    Reverse,
};
