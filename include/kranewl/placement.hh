#pragma once

#include <kranewl/geometry.hh>
#include <kranewl/decoration.hh>

#include <cstdlib>
#include <optional>

typedef class Client* Client_ptr;
struct PlacementTarget final {
    enum class TargetType
    {
        Client,
        Tab,
        Layout
    };

    TargetType type;
    union
    {
        Client_ptr client;
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
    Client_ptr client;
    Decoration decoration;
    std::optional<Region> region;
};
