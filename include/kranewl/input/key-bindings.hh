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
  {
    .action = CALL(exit()),
    .repeatable = false
  }
},

// view state modifiers
{ { XKB_KEY_c, MODKEY },
  {
    .action = CALL(kill_focus()),
    .repeatable = false
  }
},
{ { XKB_KEY_space, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_floating_focus(Toggle::Reverse)),
    .repeatable = false
  }
},
{ { XKB_KEY_f, MODKEY },
  {
    .action = CALL(set_fullscreen_focus(Toggle::Reverse)),
    .repeatable = false
  }
},
/* { { XKB_KEY_x, MODKEY }, */
/*   { */
/*     .action = CALL(set_sticky_focus(Toggle::Reverse)), */
/*     .repeatable = false */
/*   } */
/* }, */
{ { XKB_KEY_f, MODKEY | SECKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(set_contained_focus(Toggle::Reverse)),
    .repeatable = false
  }
},
{ { XKB_KEY_i, MODKEY | SECKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(set_invincible_focus(Toggle::Reverse)),
    .repeatable = false
  }
},
/* { { XKB_KEY_y, MODKEY }, */
/*   { */
/*     .action = CALL(set_iconify_focus(Toggle::Reverse)), */
/*     .repeatable = false */
/*   } */
/* }, */
/* { { XKB_KEY_u, MODKEY }, */
/*   { */
/*     .action = CALL(pop_deiconify()), */
/*     .repeatable = false */
/*   } */
/* }, */
/* { { XKB_KEY_u, MODKEY | SECKEY }, */
/*   { */
/*     .action = CALL(deiconify_all()), */
/*     .repeatable = false */
/*   } */
/* }, */

// view arrangers
{ { XKB_KEY_space, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(center_focus()),
    .repeatable = false
  }
},
{ { XKB_KEY_h, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && focus->free())
            model.nudge_focus(Edge::Left, 50);
        else
            model.shuffle_main(Direction::Backward);
    },
    .repeatable = true
  }
},
{ { XKB_KEY_j, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && focus->free())
            model.nudge_focus(Edge::Bottom, 50);
        else
            model.shuffle_stack(Direction::Forward);
    },
    .repeatable = true
  }
},
{ { XKB_KEY_k, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && focus->free())
            model.nudge_focus(Edge::Top, 50);
        else
            model.shuffle_stack(Direction::Backward);
    },
    .repeatable = true
  }
},
{ { XKB_KEY_l, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = [](Model& model) {
        View_ptr focus = model.focused_view();

        if (focus && focus->free())
            model.nudge_focus(Edge::Right, 50);
        else
            model.shuffle_main(Direction::Forward);
    },
    .repeatable = true
  }
},
{ { XKB_KEY_H, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Left, 50)),
    .repeatable = true
  }
},
{ { XKB_KEY_J, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Bottom, 50)),
    .repeatable = true
  }
},
{ { XKB_KEY_K, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Top, 50)),
    .repeatable = true
  }
},
{ { XKB_KEY_L, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Right, 50)),
    .repeatable = true
  }
},
{ { XKB_KEY_Y, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Left, -50)),
    .repeatable = true
  }
},
{ { XKB_KEY_U, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Bottom, -50)),
    .repeatable = true
  }
},
{ { XKB_KEY_I, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Top, -50)),
    .repeatable = true
  }
},
{ { XKB_KEY_O, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(stretch_focus(Edge::Right, -50)),
    .repeatable = true
  }
},
{ { XKB_KEY_Left, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(snap_focus(WLR_EDGE_LEFT)),
    .repeatable = false
  }
},
{ { XKB_KEY_Down, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(snap_focus(WLR_EDGE_BOTTOM)),
    .repeatable = false
  }
},
{ { XKB_KEY_Up, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(snap_focus(WLR_EDGE_TOP)),
    .repeatable = false
  }
},
{ { XKB_KEY_Right, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(snap_focus(WLR_EDGE_RIGHT)),
    .repeatable = false
  }
},
{ { XKB_KEY_j, MODKEY },
  {
    .action = CALL(cycle_focus_track(Direction::Forward)),
    .repeatable = true
  }
},
{ { XKB_KEY_k, MODKEY },
  {
    .action = CALL(cycle_focus_track(Direction::Backward)),
    .repeatable = true
  }
},
{ { XKB_KEY_j, MODKEY | SECKEY },
  {
    .action = CALL(cycle_focus(Direction::Forward)),
    .repeatable = true
  }
},
{ { XKB_KEY_k, MODKEY | SECKEY },
  {
    .action = CALL(cycle_focus(Direction::Backward)),
    .repeatable = true
  }
},
{ { XKB_KEY_J, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(drag_focus_track(Direction::Forward)),
    .repeatable = true
  }
},
{ { XKB_KEY_K, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(drag_focus_track(Direction::Backward)),
    .repeatable = true
  }
},
{ { XKB_KEY_a, MODKEY },
  {
    .action = CALL(cycle_track(Direction::Forward)),
    .repeatable = false
  }
},
{ { XKB_KEY_r, MODKEY },
  {
    .action = CALL(reverse_views()),
    .repeatable = false
  }
},
{ { XKB_KEY_colon, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(rotate_views(Direction::Forward)),
    .repeatable = true
  }
},
{ { XKB_KEY_less, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(rotate_views(Direction::Backward)),
    .repeatable = true
  }
},

    // workspace behavior modifiers
{ { XKB_KEY_M, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_focus_follows_cursor(Toggle::Reverse, model.mp_workspace)),
    .repeatable = false
  }
},
{ { XKB_KEY_M, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_focus_follows_cursor(Toggle::Reverse, model.mp_context)),
    .repeatable = false
  }
},

// workspace layout modifiers
{ { XKB_KEY_F, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::Float)),
    .repeatable = false
  }
},
{ { XKB_KEY_L, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::FramelessFloat)),
    .repeatable = false
  }
},
{ { XKB_KEY_z, MODKEY },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::SingleFloat)),
    .repeatable = false
  }
},
{ { XKB_KEY_Z, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::FramelessSingleFloat)),
    .repeatable = false
  }
},
{ { XKB_KEY_m, MODKEY },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::Monocle)),
    .repeatable = false
  }
},
{ { XKB_KEY_d, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::MainDeck)),
    .repeatable = false
  }
},
{ { XKB_KEY_D, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::StackDeck)),
    .repeatable = false
  }
},
{ { XKB_KEY_D, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::DoubleDeck)),
    .repeatable = false
  }
},
{ { XKB_KEY_g, MODKEY },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::Center)),
    .repeatable = false
  }
},
{ { XKB_KEY_t, MODKEY },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::DoubleStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_T, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::CompactDoubleStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_P, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::Paper)),
    .repeatable = false
  }
},
{ { XKB_KEY_P, MODKEY | SECKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::CompactPaper)),
    .repeatable = false
  }
},
{ { XKB_KEY_P, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::OverlappingPaper)),
    .repeatable = false
  }
},
{ { XKB_KEY_Y, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::HorizontalStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_y, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::CompactHorizontalStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_V, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::VerticalStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_v, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(set_layout(LayoutHandler::LayoutKind::CompactVerticalStack)),
    .repeatable = false
  }
},
{ { XKB_KEY_F, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(set_layout_retain_region(LayoutHandler::LayoutKind::Float)),
    .repeatable = false
  }
},
{ { XKB_KEY_space, MODKEY },
  {
    .action = CALL(toggle_layout()),
    .repeatable = false
  }
},
{ { XKB_KEY_Return, MODKEY },
  {
    .action = CALL_EXTERNAL(alacritty),
    .repeatable = false
  }
},

// workspace layout storage and retrieval
{ { XKB_KEY_F1, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(0)),
    .repeatable = false
  }
},
{ { XKB_KEY_F2, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(1)),
    .repeatable = false
  }
},
{ { XKB_KEY_F3, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(2)),
    .repeatable = false
  }
},
{ { XKB_KEY_F4, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(3)),
    .repeatable = false
  }
},
{ { XKB_KEY_F5, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(4)),
    .repeatable = false
  }
},
{ { XKB_KEY_F6, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(5)),
    .repeatable = false
  }
},
{ { XKB_KEY_F7, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(6)),
    .repeatable = false
  }
},
{ { XKB_KEY_F8, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(7)),
    .repeatable = false
  }
},
{ { XKB_KEY_F9, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(8)),
    .repeatable = false
  }
},
{ { XKB_KEY_F10, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(9)),
    .repeatable = false
  }
},
{ { XKB_KEY_F11, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(10)),
    .repeatable = false
  }
},
{ { XKB_KEY_F12, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(save_layout(11)),
    .repeatable = false
  }
},
{ { XKB_KEY_F1, MODKEY },
  {
    .action = CALL(load_layout(0)),
    .repeatable = false
  }
},
{ { XKB_KEY_F2, MODKEY },
  {
    .action = CALL(load_layout(1)),
    .repeatable = false
  }
},
{ { XKB_KEY_F3, MODKEY },
  {
    .action = CALL(load_layout(2)),
    .repeatable = false
  }
},
{ { XKB_KEY_F4, MODKEY },
  {
    .action = CALL(load_layout(3)),
    .repeatable = false
  }
},
{ { XKB_KEY_F5, MODKEY },
  {
    .action = CALL(load_layout(4)),
    .repeatable = false
  }
},
{ { XKB_KEY_F6, MODKEY },
  {
    .action = CALL(load_layout(5)),
    .repeatable = false
  }
},
{ { XKB_KEY_F7, MODKEY },
  {
    .action = CALL(load_layout(6)),
    .repeatable = false
  }
},
{ { XKB_KEY_F8, MODKEY },
  {
    .action = CALL(load_layout(7)),
    .repeatable = false
  }
},
{ { XKB_KEY_F9, MODKEY },
  {
    .action = CALL(load_layout(8)),
    .repeatable = false
  }
},
{ { XKB_KEY_F10, MODKEY },
  {
    .action = CALL(load_layout(9)),
    .repeatable = false
  }
},
{ { XKB_KEY_F11, MODKEY },
  {
    .action = CALL(load_layout(10)),
    .repeatable = false
  }
},
{ { XKB_KEY_F12, MODKEY },
  {
    .action = CALL(load_layout(11)),
    .repeatable = false
  }
},

// workspace layout data modifiers
{ { XKB_KEY_equal, MODKEY },
  {
    .action = CALL(change_gap_size(2)),
    .repeatable = true
  }
},
{ { XKB_KEY_minus, MODKEY },
  {
    .action = CALL(change_gap_size(-2)),
    .repeatable = true
  }
},
{ { XKB_KEY_plus, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(reset_gap_size()),
    .repeatable = true
  }
},
{ { XKB_KEY_i, MODKEY },
  {
    .action = CALL(change_main_count(1)),
    .repeatable = true
  }
},
{ { XKB_KEY_d, MODKEY },
  {
    .action = CALL(change_main_count(-1)),
    .repeatable = true
  }
},
{ { XKB_KEY_l, MODKEY },
  {
    .action = CALL(change_main_factor(.05f)),
    .repeatable = true
  }
},
{ { XKB_KEY_h, MODKEY },
  {
    .action = CALL(change_main_factor(-.05f)),
    .repeatable = true
  }
},
{ { XKB_KEY_Page_Up, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Page_Down, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(-5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Left, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Left, 5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Left, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Left, -5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Up, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Top, 5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Up, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Top, -5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Right, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Right, 5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Right, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Right, -5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Down, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Bottom, 5)),
    .repeatable = true
  }
},
{ { XKB_KEY_Down, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(change_margin(Edge::Bottom, -5)),
    .repeatable = true
  }
},
{ { XKB_KEY_less, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(cycle_layout_data(Direction::Backward)),
    .repeatable = false
  }
},
{ { XKB_KEY_greater, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(cycle_layout_data(Direction::Forward)),
    .repeatable = false
  }
},
{ { XKB_KEY_question, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(toggle_layout_data()),
    .repeatable = false
  }
},
{ { XKB_KEY_Delete, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(copy_data_from_prev_layout()),
    .repeatable = false
  }
},
{ { XKB_KEY_plus, MODKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(reset_margin()),
    .repeatable = false
  }
},
{ { XKB_KEY_plus, MODKEY | SECKEY | WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(reset_layout_data()),
    .repeatable = false
  }
},

// workspace activators
{ { XKB_KEY_Escape, MODKEY },
  {
    .action = CALL(toggle_workspace_current_context()),
    .repeatable = false
  }
},
{ { XKB_KEY_bracketright, MODKEY },
  {
    .action = CALL(activate_next_workspace_current_context(Direction::Forward)),
    .repeatable = false
  }
},
{ { XKB_KEY_bracketleft, MODKEY },
  {
    .action = CALL(activate_next_workspace_current_context(Direction::Backward)),
    .repeatable = false
  }
},
{ { XKB_KEY_1, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(Index{0})),
    .repeatable = false
  }
},
{ { XKB_KEY_2, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(1)),
    .repeatable = false
  }
},
{ { XKB_KEY_3, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(2)),
    .repeatable = false
  }
},
{ { XKB_KEY_4, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(3)),
    .repeatable = false
  }
},
{ { XKB_KEY_5, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(4)),
    .repeatable = false
  }
},
{ { XKB_KEY_6, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(5)),
    .repeatable = false
  }
},
{ { XKB_KEY_7, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(6)),
    .repeatable = false
  }
},
{ { XKB_KEY_8, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(7)),
    .repeatable = false
  }
},
{ { XKB_KEY_9, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(8)),
    .repeatable = false
  }
},
{ { XKB_KEY_0, MODKEY },
  {
    .action = CALL(activate_workspace_current_context(9)),
    .repeatable = false
  }
},

// workspace view movers
{ { XKB_KEY_braceright, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_next_workspace(Direction::Forward)),
    .repeatable = false
  }
},
{ { XKB_KEY_braceleft, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_next_workspace(Direction::Backward)),
    .repeatable = false
  }
},
{ { XKB_KEY_exclam, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(0)),
    .repeatable = false
  }
},
{ { XKB_KEY_at, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(1)),
    .repeatable = false
  }
},
{ { XKB_KEY_numbersign, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(2)),
    .repeatable = false
  }
},
{ { XKB_KEY_dollar, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(3)),
    .repeatable = false
  }
},
{ { XKB_KEY_percent, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(4)),
    .repeatable = false
  }
},
{ { XKB_KEY_asciicircum, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(5)),
    .repeatable = false
  }
},
{ { XKB_KEY_ampersand, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(6)),
    .repeatable = false
  }
},
{ { XKB_KEY_asterisk, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(7)),
    .repeatable = false
  }
},
{ { XKB_KEY_parenleft, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(8)),
    .repeatable = false
  }
},
{ { XKB_KEY_parenright, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(move_focus_to_workspace(9)),
    .repeatable = false
  }
},

// context activators
{ { XKB_KEY_Escape, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(toggle_context()),
    .repeatable = false
  }
},
{ { XKB_KEY_bracketright, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_next_context(Direction::Forward)),
    .repeatable = false
  }
},
{ { XKB_KEY_bracketleft, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_next_context(Direction::Backward)),
    .repeatable = false
  }
},
{ { XKB_KEY_1, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(Index{0})),
    .repeatable = false
  }
},
{ { XKB_KEY_2, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(1)),
    .repeatable = false
  }
},
{ { XKB_KEY_3, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(2)),
    .repeatable = false
  }
},
{ { XKB_KEY_4, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(3)),
    .repeatable = false
  }
},
{ { XKB_KEY_5, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(4)),
    .repeatable = false
  }
},
{ { XKB_KEY_6, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(5)),
    .repeatable = false
  }
},
{ { XKB_KEY_7, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(6)),
    .repeatable = false
  }
},
{ { XKB_KEY_8, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(7)),
    .repeatable = false
  }
},
{ { XKB_KEY_9, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(8)),
    .repeatable = false
  }
},
{ { XKB_KEY_0, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(activate_context(9)),
    .repeatable = false
  }
},

// view jump criteria
{ { XKB_KEY_b, MODKEY },
  {
    .action = CALL(jump_view({
        SearchSelector::SelectionCriterium::ByAppIdEquals,
        "firefox"
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_B, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL(jump_view({
        SearchSelector::SelectionCriterium::ByAppIdEquals,
        "qutebrowser"
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_b, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL(jump_view({
        SearchSelector::SelectionCriterium::ByAppIdContains,
        "chromium"
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_space, MODKEY | SECKEY },
  {
    .action = CALL(jump_view({
        SearchSelector::SelectionCriterium::ByAppIdEquals,
        "spotify"
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_e, MODKEY },
  {
    .action = CALL(jump_view({
        SearchSelector::SelectionCriterium::ByTitleContains,
        "[vim]"
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_comma, MODKEY },
  {
    .action = CALL(jump_view({
        model.mp_workspace->index(),
        Workspace::ViewSelector::SelectionCriterium::AtFirst
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_period, MODKEY },
  {
    .action = CALL(jump_view({
        model.mp_workspace->index(),
        Workspace::ViewSelector::SelectionCriterium::AtMain
    })),
    .repeatable = false
  }
},
{ { XKB_KEY_slash, MODKEY },
  {
    .action = CALL(jump_view({
        model.mp_workspace->index(),
        Workspace::ViewSelector::SelectionCriterium::AtLast
    })),
    .repeatable = false
  }
},

// external commands
{ { XKB_KEY_XF86AudioPlay, {} },
  {
    .action = CALL_EXTERNAL(playerctl play-pause),
    .repeatable = false
  }
},
{ { XKB_KEY_p, MODKEY | SECKEY },
  {
    .action = CALL_EXTERNAL(playerctl play-pause),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioPrev, {} },
  {
    .action = CALL_EXTERNAL(playerctl previous),
    .repeatable = false
  }
},
{ { XKB_KEY_k, MODKEY | SECKEY },
  {
    .action = CALL_EXTERNAL(playerctl previous),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioNext, {} },
  {
    .action = CALL_EXTERNAL(playerctl next),
    .repeatable = false
  }
},
{ { XKB_KEY_j, MODKEY | SECKEY },
  {
    .action = CALL_EXTERNAL(playerctl next),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioStop, {} },
  {
    .action = CALL_EXTERNAL(playerctl stop),
    .repeatable = false
  }
},
{ { XKB_KEY_BackSpace, MODKEY | SECKEY },
  {
    .action = CALL_EXTERNAL(playerctl stop),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioMute, {} },
  {
    .action = CALL_EXTERNAL(amixer -D pulse sset Master toggle),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioLowerVolume, {} },
  {
    .action = CALL_EXTERNAL(amixer -D pulse sset Master 5%-),
    .repeatable = true
  }
},
{ { XKB_KEY_XF86AudioRaiseVolume, {} },
  {
    .action = CALL_EXTERNAL(amixer -D pulse sset Master 5%+),
    .repeatable = true
  }
},
{ { XKB_KEY_XF86AudioMute, WLR_MODIFIER_SHIFT },
  {
    .action = CALL_EXTERNAL(amixer -D pulse sset Capture toggle),
    .repeatable = false
  }
},
{ { XKB_KEY_XF86AudioMicMute, {} },
  {
    .action = CALL_EXTERNAL(amixer -D pulse sset Capture toggle),
    .repeatable = false
  }
},
{ { XKB_KEY_space, WLR_MODIFIER_CTRL },
  {
    .action = CALL_EXTERNAL(dunstctl close),
    .repeatable = false
  }
},
{ { XKB_KEY_space, WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL_EXTERNAL(dunstctl close-all),
    .repeatable = false
  }
},
{ { XKB_KEY_less, WLR_MODIFIER_CTRL | WLR_MODIFIER_SHIFT },
  {
    .action = CALL_EXTERNAL(dunstctl history-pop),
    .repeatable = false
  }
},
{ { XKB_KEY_Return, MODKEY },
  {
    .action = CALL_EXTERNAL(alacritty),
    .repeatable = false
  }
},
{ { XKB_KEY_semicolon, MODKEY },
  {
    .action = CALL_EXTERNAL(nemo),
    .repeatable = false
  }
},
{ { XKB_KEY_p, MODKEY },
  {
    .action = CALL_EXTERNAL(rofi -show run),
    .repeatable = false
  }
},
{ { XKB_KEY_q, MODKEY },
  {
    .action = CALL_EXTERNAL(qutebrowser),
    .repeatable = false
  }
},
{ { XKB_KEY_Q, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL_EXTERNAL(firefox),
    .repeatable = false
  }
},
{ { XKB_KEY_q, MODKEY | WLR_MODIFIER_CTRL },
  {
    .action = CALL_EXTERNAL(chromium),
    .repeatable = false
  }
},
{ { XKB_KEY_apostrophe, MODKEY },
  {
    .action = CALL_EXTERNAL(blueberry),
    .repeatable = false
  }
},
{ { XKB_KEY_Return, MODKEY | WLR_MODIFIER_SHIFT },
  {
    .action = CALL_EXTERNAL(alacritty --class "kranewl:cf,Alacritty"),
    .repeatable = false
  }
},
};

}

#undef CALL_EXTERNAL
#undef CALL
#undef SECKEY
#undef MODKEY
