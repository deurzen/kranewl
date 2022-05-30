#pragma once

#ifdef XWAYLAND
extern "C" {
#include <wayland-server-core.h>
#define Cursor Cursor_
#include <xcb/xproto.h>
#undef Cursor
}

#include <array>

typedef class Server* Server_ptr;

typedef struct XWayland final {
    enum XAtom {
        NET_WM_WINDOW_TYPE_NORMAL = 0,
        NET_WM_WINDOW_TYPE_DIALOG,
        NET_WM_WINDOW_TYPE_UTILITY,
        NET_WM_WINDOW_TYPE_TOOLBAR,
        NET_WM_WINDOW_TYPE_SPLASH,
        NET_WM_WINDOW_TYPE_MENU,
        NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        NET_WM_WINDOW_TYPE_POPUP_MENU,
        NET_WM_WINDOW_TYPE_TOOLTIP,
        NET_WM_WINDOW_TYPE_NOTIFICATION,
        NET_WM_STATE_MODAL,
        XATOM_LAST,
    };

    XWayland(struct wlr_xwayland*, Server_ptr);
    ~XWayland();

    static void handle_ready(struct wl_listener*, void*);
    static void handle_new_surface(struct wl_listener*, void*);

    struct wlr_xwayland* mp_wlr_xwayland;
    struct wlr_xcursor_manager* mp_cursor_manager;

    std::array<xcb_atom_t, XATOM_LAST> m_atoms;

    Server_ptr mp_server;

    struct wl_listener ml_ready;
    struct wl_listener ml_new_surface;

}* XWayland_ptr;
#endif
