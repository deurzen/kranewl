#pragma once

#include <kranewl/input/bindings.hh>
#include <kranewl/model.hh>
#include <kranewl/layout.hh>

extern "C" {
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
#define CALL(args) [](Model& model) {model.args;}
#define CALL_EXTERNAL(command) CALL(spawn_external(#command))

namespace Bindings {

static const KeyBindings key_bindings = {
{ { XKB_KEY_Q, MODKEY | CTRL | SHIFT },
    CALL(exit())
},
{ { XKB_KEY_F, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::Float))
},
{ { XKB_KEY_L, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessFloat))
},
{ { XKB_KEY_Z, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::SingleFloat))
},
{ { XKB_KEY_Z, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessSingleFloat))
},
{ { XKB_KEY_M, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Monocle))
},
{ { XKB_KEY_D, MODKEY | CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::MainDeck))
},
{ { XKB_KEY_D, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::StackDeck))
},
{ { XKB_KEY_D, MODKEY | CTRL | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::DoubleDeck))
},
{ { XKB_KEY_G, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Center))
},
{ { XKB_KEY_T, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::DoubleStack))
},
{ { XKB_KEY_T, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactDoubleStack))
},
{ { XKB_KEY_P, MODKEY | CTRL | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::Paper))
},
{ { XKB_KEY_P, MODKEY | SECKEY | CTRL | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactPaper))
},
{ { XKB_KEY_Y, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::HorizontalStack))
},
{ { XKB_KEY_Y, MODKEY | CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactHorizontalStack))
},
{ { XKB_KEY_V, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::VerticalStack))
},
{ { XKB_KEY_V, MODKEY | CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactVerticalStack))
},
{ { XKB_KEY_F, MODKEY | CTRL | SHIFT },
    CALL(set_layout_retain_region(LayoutHandler::LayoutKind::Float))
},
{ { XKB_KEY_space, MODKEY },
    CALL(toggle_layout())
},
{ { XKB_KEY_Return, MODKEY },
    CALL_EXTERNAL(alacritty)
},
};

}
#undef CALL_EXTERNAL
#undef CALL
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
