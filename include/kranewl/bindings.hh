#pragma once

#include <functional>
#include <optional>
#include <cstdint>

class Model;
typedef class Client* Client_ptr;

typedef
    std::function<void(Model&)>
    KeyAction;

typedef
    std::function<bool(Model&, Client_ptr)>
    MouseAction;

typedef
    std::unordered_map<uint32_t, KeyAction>
    KeyBindings;

typedef
    std::unordered_map<uint32_t, MouseAction>
    MouseBindings;
