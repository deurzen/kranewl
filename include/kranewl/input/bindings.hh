#pragma once

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/cursor.hh>

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
    CursorAction;

typedef
    std::unordered_map<KeyboardInput, KeyboardAction>
    KeyBindings;

typedef
    std::unordered_map<CursorInput, CursorAction>
    CursorBindings;
