#pragma once

enum class SurfaceType {
    XDGShell,
    LayerShell,
    X11Managed,
    X11Unmanaged
};

union Surface {
    struct wlr_xdg_surface* xdg;
    struct wlr_xwayland_surface* xwayland;
};
