#pragma once

#include <cstddef>

typedef unsigned Pid;
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

enum class Toggle
{
    On,
    Off,
    Reverse,
};
