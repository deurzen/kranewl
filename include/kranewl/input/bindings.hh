#pragma once

#include <kranewl/input/keyboard.hh>
#include <kranewl/input/cursor.hh>

#include <cstdint>
#include <optional>

class Manager;
typedef struct View* View_ptr;

typedef
    std::unordered_map<KeyboardInput, KeyboardAction>
    KeyBindings;

typedef
    std::unordered_map<CursorInput, CursorAction>
    CursorBindings;
