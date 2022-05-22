#pragma once

#include <kranewl/tree/view.hh>

struct SubsurfaceViewChild final : public ViewChild {
    SubsurfaceViewChild();
    ~SubsurfaceViewChild();

    struct wl_listener ml_destroy;

};
