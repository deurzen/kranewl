#pragma once

#include <kranewl/input/bindings.hh>
#include <kranewl/layout.hh>
#include <kranewl/model.hh>

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

// view state modifiers
{ { XKB_KEY_c, MODKEY },
    CALL(kill_focus())
},
{ { XKB_KEY_space, MODKEY | SHIFT },
    CALL(set_floating_focus(Toggle::Reverse))
},
{ { XKB_KEY_f, MODKEY },
    CALL(set_fullscreen_focus(Toggle::Reverse))
},
{ { XKB_KEY_x, MODKEY },
    CALL(set_sticky_focus(Toggle::Reverse))
},
{ { XKB_KEY_f, MODKEY | SECKEY | CTRL },
    CALL(set_contained_focus(Toggle::Reverse))
},
{ { XKB_KEY_i, MODKEY | SECKEY | CTRL },
    CALL(set_invincible_focus(Toggle::Reverse))
},
{ { XKB_KEY_y, MODKEY },
    CALL(set_iconify_focus(Toggle::Reverse))
},
{ { XKB_KEY_u, MODKEY },
    CALL(pop_deiconify())
},
{ { XKB_KEY_u, MODKEY | SECKEY },
    CALL(deiconify_all())
},

// view arrangers
{ { XKB_KEY_space, MODKEY | CTRL },
    CALL(center_focus())
},
{ { XKB_KEY_h, MODKEY | CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Left, 15);
        else
            model.shuffle_main(Direction::Backward);
    }
},
{ { XKB_KEY_j, MODKEY | CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Bottom, 15);
        else
            model.shuffle_stack(Direction::Forward);
    }
},
{ { XKB_KEY_k, MODKEY | CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Top, 15);
        else
            model.shuffle_stack(Direction::Backward);
    }
},
{ { XKB_KEY_l, MODKEY | CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Right, 15);
        else
            model.shuffle_main(Direction::Forward);
    }
},
{ { XKB_KEY_H, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Left, 15))
},
{ { XKB_KEY_J, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Bottom, 15))
},
{ { XKB_KEY_K, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Top, 15))
},
{ { XKB_KEY_L, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Right, 15))
},
{ { XKB_KEY_Y, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Left, -15))
},
{ { XKB_KEY_U, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Bottom, -15))
},
{ { XKB_KEY_I, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Top, -15))
},
{ { XKB_KEY_O, MODKEY | CTRL | SHIFT },
    CALL(stretch_focus(Edge::Right, -15))
},
{ { XKB_KEY_leftarrow, MODKEY | CTRL },
    CALL(snap_focus(Edge::Left))
},
{ { XKB_KEY_downarrow, MODKEY | CTRL },
    CALL(snap_focus(Edge::Bottom))
},
{ { XKB_KEY_uparrow, MODKEY | CTRL },
    CALL(snap_focus(Edge::Top))
},
{ { XKB_KEY_rightarrow, MODKEY | CTRL },
    CALL(snap_focus(Edge::Right))
},
{ { XKB_KEY_j, MODKEY },
    CALL(cycle_focus(Direction::Forward))
},
{ { XKB_KEY_k, MODKEY },
    CALL(cycle_focus(Direction::Backward))
},
{ { XKB_KEY_J, MODKEY | SHIFT },
    CALL(drag_focus(Direction::Forward))
},
{ { XKB_KEY_K, MODKEY | SHIFT },
    CALL(drag_focus(Direction::Backward))
},
{ { XKB_KEY_r, MODKEY },
    CALL(reverse_views())
},
{ { XKB_KEY_semicolon, MODKEY | SHIFT },
    CALL(rotate_views(Direction::Forward))
},
{ { XKB_KEY_comma, MODKEY | SHIFT },
    CALL(rotate_views(Direction::Backward))
},

// workspace layout modifiers
{ { XKB_KEY_F, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::Float))
},
{ { XKB_KEY_L, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessFloat))
},
{ { XKB_KEY_z, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::SingleFloat))
},
{ { XKB_KEY_Z, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessSingleFloat))
},
{ { XKB_KEY_m, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Monocle))
},
{ { XKB_KEY_d, MODKEY | CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::MainDeck))
},
{ { XKB_KEY_D, MODKEY | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::StackDeck))
},
{ { XKB_KEY_D, MODKEY | CTRL | SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::DoubleDeck))
},
{ { XKB_KEY_g, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Center))
},
{ { XKB_KEY_t, MODKEY },
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
{ { XKB_KEY_y, MODKEY | CTRL },
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

// workspace layout data modifiers
{ { XKB_KEY_equal, MODKEY },
    CALL(change_gap_size(2))
},
{ { XKB_KEY_minus, MODKEY },
    CALL(change_gap_size(-2))
},
{ { XKB_KEY_equal, MODKEY | SHIFT },
    CALL(reset_gap_size())
},
{ { XKB_KEY_i, MODKEY },
    CALL(change_main_count(1))
},
{ { XKB_KEY_d, MODKEY },
    CALL(change_main_count(-1))
},
{ { XKB_KEY_l, MODKEY },
    CALL(change_main_factor(.05f))
},
{ { XKB_KEY_h, MODKEY },
    CALL(change_main_factor(-.05f))
},
{ { XKB_KEY_Page_Up, MODKEY | SHIFT },
    CALL(change_margin(5))
},
{ { XKB_KEY_Page_Down, MODKEY | SHIFT },
    CALL(change_margin(-5))
},
{ { XKB_KEY_Left, MODKEY | SHIFT },
    CALL(change_margin(Edge::Left, 5))
},
{ { XKB_KEY_Left, MODKEY | CTRL | SHIFT },
    CALL(change_margin(Edge::Left, -5))
},
{ { XKB_KEY_Up, MODKEY | SHIFT },
    CALL(change_margin(Edge::Top, 5))
},
{ { XKB_KEY_Up, MODKEY | CTRL | SHIFT },
    CALL(change_margin(Edge::Top, -5))
},
{ { XKB_KEY_Right, MODKEY | SHIFT },
    CALL(change_margin(Edge::Right, 5))
},
{ { XKB_KEY_Right, MODKEY | CTRL | SHIFT },
    CALL(change_margin(Edge::Right, -5))
},
{ { XKB_KEY_Down, MODKEY | SHIFT },
    CALL(change_margin(Edge::Bottom, 5))
},
{ { XKB_KEY_Down, MODKEY | CTRL | SHIFT },
    CALL(change_margin(Edge::Bottom, -5))
},
{ { XKB_KEY_comma, MODKEY | CTRL | SHIFT },
    CALL(cycle_layout_data(Direction::Backward))
},
{ { XKB_KEY_period, MODKEY | CTRL | SHIFT },
    CALL(cycle_layout_data(Direction::Forward))
},
{ { XKB_KEY_slash, MODKEY | CTRL | SHIFT },
    CALL(toggle_layout_data())
},
{ { XKB_KEY_Delete, MODKEY | CTRL | SHIFT },
    CALL(copy_data_from_prev_layout())
},
{ { XKB_KEY_equal, MODKEY | CTRL | SHIFT },
    CALL(reset_margin())
},
{ { XKB_KEY_equal, MODKEY | SECKEY | CTRL | SHIFT },
    CALL(reset_layout_data())
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
