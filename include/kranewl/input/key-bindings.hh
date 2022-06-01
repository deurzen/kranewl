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

#define CALL(args) [](Model& model) {model.args;}
#define CALL_EXTERNAL(command) CALL(spawn_external(#command))

namespace Bindings {

static const KeyBindings key_bindings = {
{ { XKB_KEY_Q, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(exit())
},

// view state modifiers
{ { XKB_KEY_c, MODKEY },
    CALL(kill_focus())
},
{ { XKB_KEY_space, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_floating_focus(Toggle::Reverse))
},
{ { XKB_KEY_f, MODKEY },
    CALL(set_fullscreen_focus(Toggle::Reverse))
},
{ { XKB_KEY_x, MODKEY },
    CALL(set_sticky_focus(Toggle::Reverse))
},
{ { XKB_KEY_f, MODKEY | SECKEY | WLR_MODIFIER_CTRL },
    CALL(set_contained_focus(Toggle::Reverse))
},
{ { XKB_KEY_i, MODKEY | SECKEY | WLR_MODIFIER_CTRL },
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
{ { XKB_KEY_space, MODKEY | WLR_MODIFIER_CTRL },
    CALL(center_focus())
},
{ { XKB_KEY_h, MODKEY | WLR_MODIFIER_CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Left, 15);
        else
            model.shuffle_main(Direction::Backward);
    }
},
{ { XKB_KEY_j, MODKEY | WLR_MODIFIER_CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Bottom, 15);
        else
            model.shuffle_stack(Direction::Forward);
    }
},
{ { XKB_KEY_k, MODKEY | WLR_MODIFIER_CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Top, 15);
        else
            model.shuffle_stack(Direction::Backward);
    }
},
{ { XKB_KEY_l, MODKEY | WLR_MODIFIER_CTRL },
    [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && model.is_free(focus))
            model.nudge_focus(Edge::Right, 15);
        else
            model.shuffle_main(Direction::Forward);
    }
},
{ { XKB_KEY_H, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Left, 15))
},
{ { XKB_KEY_J, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Bottom, 15))
},
{ { XKB_KEY_K, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Top, 15))
},
{ { XKB_KEY_L, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Right, 15))
},
{ { XKB_KEY_Y, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Left, -15))
},
{ { XKB_KEY_U, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Bottom, -15))
},
{ { XKB_KEY_I, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Top, -15))
},
{ { XKB_KEY_O, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(stretch_focus(Edge::Right, -15))
},
{ { XKB_KEY_leftarrow, MODKEY | WLR_MODIFIER_CTRL },
    CALL(snap_focus(Edge::Left))
},
{ { XKB_KEY_downarrow, MODKEY | WLR_MODIFIER_CTRL },
    CALL(snap_focus(Edge::Bottom))
},
{ { XKB_KEY_uparrow, MODKEY | WLR_MODIFIER_CTRL },
    CALL(snap_focus(Edge::Top))
},
{ { XKB_KEY_rightarrow, MODKEY | WLR_MODIFIER_CTRL },
    CALL(snap_focus(Edge::Right))
},
{ { XKB_KEY_j, MODKEY },
    CALL(cycle_focus(Direction::Forward))
},
{ { XKB_KEY_k, MODKEY },
    CALL(cycle_focus(Direction::Backward))
},
{ { XKB_KEY_J, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(drag_focus(Direction::Forward))
},
{ { XKB_KEY_K, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(drag_focus(Direction::Backward))
},
{ { XKB_KEY_r, MODKEY },
    CALL(reverse_views())
},
{ { XKB_KEY_semicolon, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(rotate_views(Direction::Forward))
},
{ { XKB_KEY_comma, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(rotate_views(Direction::Backward))
},

    // workspace behavior modifiers
{ { XKB_KEY_M, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_focus_follows_cursor(Toggle::Reverse, model.mp_workspace))
},
{ { XKB_KEY_M, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(set_focus_follows_cursor(Toggle::Reverse, model.mp_context))
},

// workspace layout modifiers
{ { XKB_KEY_F, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::Float))
},
{ { XKB_KEY_L, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessFloat))
},
{ { XKB_KEY_z, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::SingleFloat))
},
{ { XKB_KEY_Z, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::FramelessSingleFloat))
},
{ { XKB_KEY_m, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Monocle))
},
{ { XKB_KEY_d, MODKEY | WLR_MODIFIER_CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::MainDeck))
},
{ { XKB_KEY_D, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::StackDeck))
},
{ { XKB_KEY_D, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::DoubleDeck))
},
{ { XKB_KEY_g, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::Center))
},
{ { XKB_KEY_t, MODKEY },
    CALL(set_layout(LayoutHandler::LayoutKind::DoubleStack))
},
{ { XKB_KEY_T, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactDoubleStack))
},
{ { XKB_KEY_P, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::Paper))
},
{ { XKB_KEY_P, MODKEY | SECKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactPaper))
},
{ { XKB_KEY_Y, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::HorizontalStack))
},
{ { XKB_KEY_y, MODKEY | WLR_MODIFIER_CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactHorizontalStack))
},
{ { XKB_KEY_V, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(set_layout(LayoutHandler::LayoutKind::VerticalStack))
},
{ { XKB_KEY_V, MODKEY | WLR_MODIFIER_CTRL },
    CALL(set_layout(LayoutHandler::LayoutKind::CompactVerticalStack))
},
{ { XKB_KEY_F, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(set_layout_retain_region(LayoutHandler::LayoutKind::Float))
},
{ { XKB_KEY_space, MODKEY },
    CALL(toggle_layout())
},
{ { XKB_KEY_Return, MODKEY },
    CALL_EXTERNAL(alacritty)
},

// workspace layout storage and retrieval
{ { XKB_KEY_F1, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(0))
},
{ { XKB_KEY_F2, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(1))
},
{ { XKB_KEY_F3, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(2))
},
{ { XKB_KEY_F4, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(3))
},
{ { XKB_KEY_F5, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(4))
},
{ { XKB_KEY_F6, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(5))
},
{ { XKB_KEY_F7, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(6))
},
{ { XKB_KEY_F8, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(7))
},
{ { XKB_KEY_F9, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(8))
},
{ { XKB_KEY_F10, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(9))
},
{ { XKB_KEY_F11, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(10))
},
{ { XKB_KEY_F12, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(save_layout(11))
},
{ { XKB_KEY_F1, MODKEY },
    CALL(load_layout(0))
},
{ { XKB_KEY_F2, MODKEY },
    CALL(load_layout(1))
},
{ { XKB_KEY_F3, MODKEY },
    CALL(load_layout(2))
},
{ { XKB_KEY_F4, MODKEY },
    CALL(load_layout(3))
},
{ { XKB_KEY_F5, MODKEY },
    CALL(load_layout(4))
},
{ { XKB_KEY_F6, MODKEY },
    CALL(load_layout(5))
},
{ { XKB_KEY_F7, MODKEY },
    CALL(load_layout(6))
},
{ { XKB_KEY_F8, MODKEY },
    CALL(load_layout(7))
},
{ { XKB_KEY_F9, MODKEY },
    CALL(load_layout(8))
},
{ { XKB_KEY_F10, MODKEY },
    CALL(load_layout(9))
},
{ { XKB_KEY_F11, MODKEY },
    CALL(load_layout(10))
},
{ { XKB_KEY_F12, MODKEY },
    CALL(load_layout(11))
},

// workspace layout data modifiers
{ { XKB_KEY_equal, MODKEY },
    CALL(change_gap_size(2))
},
{ { XKB_KEY_minus, MODKEY },
    CALL(change_gap_size(-2))
},
{ { XKB_KEY_equal, MODKEY | WLR_MODIFIER_SHIFT },
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
{ { XKB_KEY_Page_Up, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(5))
},
{ { XKB_KEY_Page_Down, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(-5))
},
{ { XKB_KEY_Left, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Left, 5))
},
{ { XKB_KEY_Left, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Left, -5))
},
{ { XKB_KEY_Up, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Top, 5))
},
{ { XKB_KEY_Up, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Top, -5))
},
{ { XKB_KEY_Right, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Right, 5))
},
{ { XKB_KEY_Right, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Right, -5))
},
{ { XKB_KEY_Down, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Bottom, 5))
},
{ { XKB_KEY_Down, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(change_margin(Edge::Bottom, -5))
},
{ { XKB_KEY_comma, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(cycle_layout_data(Direction::Backward))
},
{ { XKB_KEY_period, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(cycle_layout_data(Direction::Forward))
},
{ { XKB_KEY_slash, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(toggle_layout_data())
},
{ { XKB_KEY_Delete, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(copy_data_from_prev_layout())
},
{ { XKB_KEY_equal, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(reset_margin())
},
{ { XKB_KEY_equal, MODKEY | SECKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
    CALL(reset_layout_data())
},

// workspace activators
{ { XKB_KEY_Escape, MODKEY },
    CALL(toggle_workspace_current_context())
},
{ { XKB_KEY_bracketright, MODKEY },
    CALL(activate_next_workspace_current_context(Direction::Forward))
},
{ { XKB_KEY_bracketleft, MODKEY },
    CALL(activate_next_workspace_current_context(Direction::Backward))
},
{ { XKB_KEY_1, MODKEY },
    CALL(activate_workspace_current_context(Index{0}))
},
{ { XKB_KEY_2, MODKEY },
    CALL(activate_workspace_current_context(1))
},
{ { XKB_KEY_3, MODKEY },
    CALL(activate_workspace_current_context(2))
},
{ { XKB_KEY_4, MODKEY },
    CALL(activate_workspace_current_context(3))
},
{ { XKB_KEY_5, MODKEY },
    CALL(activate_workspace_current_context(4))
},
{ { XKB_KEY_6, MODKEY },
    CALL(activate_workspace_current_context(5))
},
{ { XKB_KEY_7, MODKEY },
    CALL(activate_workspace_current_context(6))
},
{ { XKB_KEY_8, MODKEY },
    CALL(activate_workspace_current_context(7))
},
{ { XKB_KEY_9, MODKEY },
    CALL(activate_workspace_current_context(8))
},
{ { XKB_KEY_0, MODKEY },
    CALL(activate_workspace_current_context(9))
},

// workspace client movers
{ { XKB_KEY_braceright, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_next_workspace(Direction::Forward))
},
{ { XKB_KEY_braceleft, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_next_workspace(Direction::Backward))
},
{ { XKB_KEY_exclam, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(0))
},
{ { XKB_KEY_at, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(1))
},
{ { XKB_KEY_numbersign, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(2))
},
{ { XKB_KEY_dollar, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(3))
},
{ { XKB_KEY_percent, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(4))
},
{ { XKB_KEY_asciicircum, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(5))
},
{ { XKB_KEY_ampersand, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(6))
},
{ { XKB_KEY_asterisk, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(7))
},
{ { XKB_KEY_parenleft, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(8))
},
{ { XKB_KEY_parenright, MODKEY | WLR_MODIFIER_SHIFT },
    CALL(move_focus_to_workspace(9))
},

// context activators
{ { XKB_KEY_Escape, MODKEY | WLR_MODIFIER_CTRL },
    CALL(toggle_context())
},
{ { XKB_KEY_bracketright, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_next_context(Direction::Forward))
},
{ { XKB_KEY_bracketleft, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_next_context(Direction::Backward))
},
{ { XKB_KEY_1, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(Index{0}))
},
{ { XKB_KEY_2, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(1))
},
{ { XKB_KEY_3, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(2))
},
{ { XKB_KEY_4, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(3))
},
{ { XKB_KEY_5, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(4))
},
{ { XKB_KEY_6, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(5))
},
{ { XKB_KEY_7, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(6))
},
{ { XKB_KEY_8, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(7))
},
{ { XKB_KEY_9, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(8))
},
{ { XKB_KEY_0, MODKEY | WLR_MODIFIER_CTRL },
    CALL(activate_context(9))
},
};

}

#undef CALL_EXTERNAL
#undef CALL
#undef SECKEY
#undef MODKEY
