#pragma once

#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

#include <cstdint>

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;

typedef struct XDGView final : public View {
    XDGView(
        struct wlr_xdg_surface*,
        Server_ptr,
        Model_ptr,
        Seat_ptr
    );

    ~XDGView();

    static void handle_foreign_activate_request(struct wl_listener*, void*);
    static void handle_foreign_fullscreen_request(struct wl_listener*, void*);
    static void handle_foreign_close_request(struct wl_listener*, void*);
    static void handle_foreign_destroy(struct wl_listener*, void*);
    static void handle_surface_new_subsurface(struct wl_listener*, void*);
    static void handle_commit(struct wl_listener*, void*);
    static void handle_request_move(struct wl_listener*, void*);
    static void handle_request_resize(struct wl_listener*, void*);
    static void handle_request_fullscreen(struct wl_listener*, void*);
    static void handle_set_title(struct wl_listener*, void*);
    static void handle_set_app_id(struct wl_listener*, void*);
    static void handle_new_popup(struct wl_listener*, void*);
    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    struct wlr_xdg_surface* mp_wlr_xdg_surface;
    struct wlr_xdg_toplevel* mp_wlr_xdg_toplevel;

    struct wl_listener ml_commit;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_set_title;
    struct wl_listener ml_set_app_id;
    struct wl_listener ml_new_popup;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;

}* XDGView_ptr;
