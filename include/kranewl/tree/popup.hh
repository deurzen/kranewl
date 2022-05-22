#pragma once

#include <kranewl/tree/view.hh>

struct PopupViewChild final : public ViewChild {
    PopupViewChild();
    ~PopupViewChild();

    struct wlr_xdg_popup* mp_wlr_xdg_popup;

    struct wl_listener ml_new_popup;
    struct wl_listener ml_destroy;

};
