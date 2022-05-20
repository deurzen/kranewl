#pragma once

#include <kranewl/common.hh>

enum class SurfaceType {
    XDGShell,
    LayerShell,
    X11Managed,
    X11Unmanaged
};

struct Surface {
    Surface(struct wlr_xdg_surface* xdg, bool toplevel)
      : type{toplevel
            ? SurfaceType::XDGShell
            : SurfaceType::LayerShell},
        xdg{xdg}
    {}

    Surface(struct wlr_xwayland_surface* xwayland, bool managed)
      : type{managed
            ? SurfaceType::X11Managed
            : SurfaceType::X11Unmanaged},
        xwayland{xwayland}
    {}

    Uid
    uid() const
    {
        switch (type) {
        case SurfaceType::XDGShell: // fallthrough
        case SurfaceType::LayerShell: return reinterpret_cast<Uid>(xdg);
        case SurfaceType::X11Managed: // fallthrough
        case SurfaceType::X11Unmanaged: return reinterpret_cast<Uid>(xwayland);
        default: break;
        }

        return 0;
    }

    SurfaceType type;
    union {
        struct wlr_xdg_surface* xdg;
        struct wlr_xwayland_surface* xwayland;
    };
};
