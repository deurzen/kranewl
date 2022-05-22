#pragma once

#include <kranewl/geometry.hh>
#include <kranewl/decoration.hh>

#include <cstdlib>
#include <optional>

typedef class View* View_ptr;
struct PlacementTarget final {
    enum class TargetType
    {
        View,
        Tab,
        Layout
    };

    TargetType type;
    union
    {
        View_ptr view;
        std::size_t tab;
    };
};

struct Placement final {
    enum class PlacementMethod
    {
        Free,
        Tile,
    };

    PlacementMethod method;
    View_ptr view;
    Decoration decoration;
    std::optional<Region> region;
};
