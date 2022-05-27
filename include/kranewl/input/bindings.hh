#pragma once

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/mouse.hh>

#include <cstdint>
#include <functional>
#include <optional>

class Model;
typedef struct View* View_ptr;

typedef
    std::function<void(Model&)>
    KeyboardAction;

typedef
    std::function<bool(Model&, View_ptr)>
    MouseAction;

typedef
    std::unordered_map<KeyboardInput, KeyboardAction>
    KeyBindings;

typedef
    std::unordered_map<MouseInput, MouseAction>
    MouseBindings;
