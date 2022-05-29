#pragma once

#include <kranewl/input/bindings.hh>
#include <kranewl/input/cursor.hh>
#include <kranewl/layout.hh>
#include <kranewl/model.hh>

extern "C" {
#include <linux/input-event-codes.h>
#include <wlr/types/wlr_keyboard.h>
}

#ifdef NDEBUG
#define MODKEY WLR_MODIFIER_LOGO
#define SECKEY WLR_MODIFIER_ALT
#else
#define MODKEY WLR_MODIFIER_ALT
#define SECKEY WLR_MODIFIER_LOGO
#endif
#define SHIFT WLR_MODIFIER_SHIFT
#define CAPS WLR_MODIFIER_CAPS
#define CTRL WLR_MODIFIER_CTRL
#define ALT WLR_MODIFIER_ALT
#define MOD2 WLR_MODIFIER_MOD2
#define MOD3 WLR_MODIFIER_MOD3
#define LOGO WLR_MODIFIER_LOGO
#define MOD5 WLR_MODIFIER_MOD5
#define GLOBAL CursorInput::Target::Global
#define ROOT CursorInput::Target::Root
#define VIEW CursorInput::Target::View
#define LEFT CursorInput::Button::Left
#define RIGHT CursorInput::Button::Right
#define MIDDLE CursorInput::Button::Middle
#define SCROLLUP CursorInput::Button::ScrollUp
#define SCROLLDOWN CursorInput::Button::ScrollDown
#define SCROLLLEFT CursorInput::Button::ScrollLeft
#define SCROLLRIGHT CursorInput::Button::ScrollRight
#define FORWARD CursorInput::Button::Forward
#define BACKWARD CursorInput::Button::Backward
#define CALL_FOCUS(args) [](Model& model, View_ptr view) {{args} return true;}
#define CALL_NOFOCUS(args) [](Model& model, View_ptr view) {{args} return false;}
#define CALL_EXTERNAL(command) CALL(spawn_external(#command))

namespace Bindings {

static const CursorBindings cursor_bindings = {
{ { VIEW, RIGHT, MODKEY | CTRL },
    CALL_FOCUS({
        if (view)
            model.set_floating_view(Toggle::Reverse, view);
    })
},
{ { VIEW, MIDDLE, MODKEY | CTRL | SHIFT },
    CALL_FOCUS({
        if (view)
            model.set_fullscreen_view(Toggle::Reverse, view);
    })
},
{ { VIEW, MIDDLE, MODKEY },
    CALL_FOCUS({
        if (view)
            model.center_view(view);
    })
},
{ { VIEW, SCROLLDOWN, MODKEY | CTRL | SHIFT },
    CALL_FOCUS({
        if (view)
            model.inflate_view(-16, view);
    })
},
{ { VIEW, SCROLLUP, MODKEY | CTRL | SHIFT },
    CALL_FOCUS({
        if (view)
            model.inflate_view(16, view);
    })
},
{ { VIEW, LEFT, MODKEY },
    CALL_FOCUS({
        if (view)
            model.cursor_interactive(Cursor::Mode::Move, view);
    })
},
{ { VIEW, RIGHT, MODKEY },
    CALL_FOCUS({
        if (view)
            model.cursor_interactive(Cursor::Mode::Resize, view);
    })
},
{ { GLOBAL, SCROLLDOWN, MODKEY },
    CALL_NOFOCUS({
        model.cycle_focus(Direction::Forward);
    })
},
{ { GLOBAL, SCROLLUP, MODKEY },
    CALL_NOFOCUS({
        model.cycle_focus(Direction::Backward);
    })
},
{ { GLOBAL, SCROLLDOWN, MODKEY | SHIFT },
    CALL_NOFOCUS({
        /* model.activate_next_workspace(Direction::Forward); */
    })
},
{ { GLOBAL, SCROLLUP, MODKEY | SHIFT },
    CALL_NOFOCUS({
        /* model.activate_next_workspace(Direction::Backward); */
    })
},
{ { VIEW, FORWARD, MODKEY },
    CALL_NOFOCUS({
        /* if (view) */
        /*     model.move_view_to_next_workspace(Direction::Forward, view); */
    })
},
{ { VIEW, BACKWARD, MODKEY },
    CALL_NOFOCUS({
        /* if (view) */
        /*     model.move_view_to_next_workspace(Direction::Backward, view); */
    })
},
{ { VIEW, RIGHT, MODKEY | CTRL | SHIFT },
    CALL_NOFOCUS({
        if (view)
            model.kill_view(view);
    })
},
{ { GLOBAL, LEFT, MODKEY | SECKEY | CTRL },
    CALL_NOFOCUS({
        model.spawn_external("alacritty --class kranewl:cf,Alacritty");
    })
},
};

}

#undef CALL_EXTERNAL
#undef CALL_NOFOCUS
#undef CALL_FOCUS
#undef BACKWARD
#undef FORWARD
#undef SCROLLRIGHT
#undef SCROLLLEFT
#undef SCROLLDOWN
#undef SCROLLUP
#undef MIDDLE
#undef RIGHT
#undef LEFT
#undef VIEW
#undef ROOT
#undef GLOBAL
#undef MOD5
#undef LOGO
#undef MOD3
#undef MOD2
#undef ALT
#undef CTRL
#undef CAPS
#undef SHIFT
#undef SECKEY
#undef MODKEY
#undef SECKEY
#undef MODKEY
