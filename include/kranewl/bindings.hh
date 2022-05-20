#pragma once

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/mouse.hh>

#include <functional>
#include <optional>
#include <cstdint>

class Model;
typedef class Client* Client_ptr;

typedef
    std::function<void(Model&)>
    KeyboardAction;

typedef
    std::function<bool(Model&, Client_ptr)>
    MouseAction;

typedef
    std::unordered_map<KeyboardInput, KeyboardAction>
    KeyBindings;

typedef
    std::unordered_map<MouseInput, MouseAction>
    MouseBindings;
