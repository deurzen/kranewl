#include <trace.hh>

#include <kranewl/input/seat.hh>
#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/xwayland-view.hh>
#include <kranewl/xwayland.hh>

// https://github.com/swaywm/wlroots/issues/682
#include <pthread.h>
#define class class_
#define namespace namespace_
#define static
extern "C" {
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/xwayland.h>
#define Cursor Cursor_
#include <X11/Xlib.h>
#undef Cursor
}
#undef static
#undef namespace
#undef class

#include <spdlog/spdlog.h>

#include <cstdlib>

XWayland::XWayland(
    struct wlr_xwayland* xwayland,
    Server_ptr server,
    Model_ptr model,
    Seat_ptr seat
)
    : mp_wlr_xwayland(xwayland),
      mp_cursor_manager(wlr_xcursor_manager_create(nullptr, 24)),
      m_atoms({}),
      mp_server(server),
      mp_model(model),
      mp_seat(seat),
      ml_ready({ .notify = XWayland::handle_ready }),
      ml_new_surface({ .notify = XWayland::handle_new_surface })
{
    wlr_xcursor_manager_load(mp_cursor_manager, 1.f);

    if (mp_wlr_xwayland) {
        setenv("DISPLAY", mp_wlr_xwayland->display_name, 1);
        wl_signal_add(&mp_wlr_xwayland->events.ready, &ml_ready);
        wl_signal_add(&mp_wlr_xwayland->events.new_surface, &ml_new_surface);
    } else {
        spdlog::error("Failed to initiate XWayland");
        spdlog::warn("Continuing without XWayland functionality");
    }
}

XWayland::~XWayland()
{}

static inline constexpr std::array<char const*, XWayland::XATOM_LAST>
constexpr_atom_names()
{
    std::array<char const*, XWayland::XATOM_LAST> atom_names_ = {};

    atom_names_[XWayland::NET_WM_WINDOW_TYPE_NORMAL] = "_NET_WM_WINDOW_TYPE_NORMAL";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_DIALOG] = "_NET_WM_WINDOW_TYPE_DIALOG";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_UTILITY] = "_NET_WM_WINDOW_TYPE_UTILITY";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_TOOLBAR] = "_NET_WM_WINDOW_TYPE_TOOLBAR";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_SPLASH] = "_NET_WM_WINDOW_TYPE_SPLASH";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_MENU] = "_NET_WM_WINDOW_TYPE_MENU";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_DROPDOWN_MENU] = "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_POPUP_MENU] = "_NET_WM_WINDOW_TYPE_POPUP_MENU";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_TOOLTIP] = "_NET_WM_WINDOW_TYPE_TOOLTIP";
    atom_names_[XWayland::NET_WM_WINDOW_TYPE_NOTIFICATION] = "_NET_WM_WINDOW_TYPE_NOTIFICATION";
    atom_names_[XWayland::NET_WM_STATE_MODAL] = "_NET_WM_STATE_MODAL";

    return atom_names_;
}

void
XWayland::handle_ready(struct wl_listener* listener, void*)
{
    TRACE();

    static constexpr std::array<char const*, XATOM_LAST> atom_names = constexpr_atom_names();

    XWayland_ptr xwayland = wl_container_of(listener, xwayland, ml_ready);
    xcb_connection_t* xconn = xcb_connect(xwayland->mp_wlr_xwayland->display_name, nullptr);

    int error = xcb_connection_has_error(xconn);
    if (error) {
        spdlog::error("Establishing connection with the X server failed with X11 error code {}", error);
        spdlog::warn("Continuing without XWayland support");
        return;
    }

    xcb_intern_atom_cookie_t atom_cookies[XATOM_LAST];
    for (std::size_t i = 0; i < XATOM_LAST; ++i)
        atom_cookies[i]
            = xcb_intern_atom(xconn, 0, strlen(atom_names[i]), atom_names[i]);

    for (std::size_t i = 0; i < XATOM_LAST; ++i) {
        xcb_generic_error_t* error = NULL;
        xcb_intern_atom_reply_t *reply
            = xcb_intern_atom_reply(xconn, atom_cookies[i], &error);

        if (reply && !error)
            xwayland->m_atoms[i] = reply->atom;

        free(reply);

        if (error) {
            spdlog::error(
                "Interning atom {} failed with X11 error code {}",
                atom_names[i],
                error->error_code
            );
            spdlog::warn("Continuing with limited XWayland support");

            free(error);
            break;
        }
    }

    wlr_xwayland_set_seat(
        xwayland->mp_wlr_xwayland,
        xwayland->mp_seat->mp_wlr_seat
    );

    struct wlr_xcursor* xcursor;
    if ((xcursor = wlr_xcursor_manager_get_xcursor(xwayland->mp_cursor_manager, "left_ptr", 1)))
        wlr_xwayland_set_cursor(
            xwayland->mp_wlr_xwayland,
            xcursor->images[0]->buffer,
            xcursor->images[0]->width * 4,
            xcursor->images[0]->width,
            xcursor->images[0]->height,
            xcursor->images[0]->hotspot_x,
            xcursor->images[0]->hotspot_y
        );

    xcb_disconnect(xconn);
    spdlog::info("Initiated underlying XWayland server");
}

void
XWayland::handle_new_surface(struct wl_listener* listener, void* data)
{
    TRACE();

    XWayland_ptr xwayland = wl_container_of(listener, xwayland, ml_new_surface);
    struct wlr_xwayland_surface* xwayland_surface
        = reinterpret_cast<struct wlr_xwayland_surface*>(data);

    XWaylandView_ptr xwayland_view = xwayland->mp_model->create_xwayland_view(
        xwayland_surface,
        xwayland->mp_seat,
        xwayland
    );
}
