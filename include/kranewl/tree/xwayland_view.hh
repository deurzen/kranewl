#pragma once

#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;

#ifdef XWAYLAND
typedef struct XWaylandView final : public View {
    XWaylandView(
        struct wlr_xwayland_surface*,
        Server_ptr,
        Model_ptr,
        Seat_ptr
    );

    ~XWaylandView();

    Region constraints() override;
    pid_t pid() override;
    bool prefers_floating() override;
    View_ptr is_transient_for() override;

    void map() override;
    void unmap() override;
    void focus(Toggle) override;
    void activate(Toggle) override;
    void set_tiled(Toggle) override;
    void set_fullscreen(Toggle) override;
    void set_resizing(Toggle) override;

    void configure(Region const&, Extents const&, bool) override;
    void close() override;
    void close_popups() override;
    void destroy() override;

    static void handle_commit(struct wl_listener*, void*);
    static void handle_request_move(struct wl_listener*, void*);
    static void handle_request_resize(struct wl_listener*, void*);
    static void handle_request_maximize(struct wl_listener*, void*);
    static void handle_request_minimize(struct wl_listener*, void*);
    static void handle_request_configure(struct wl_listener*, void*);
    static void handle_request_fullscreen(struct wl_listener*, void*);
    static void handle_request_activate(struct wl_listener*, void*);
    static void handle_set_title(struct wl_listener*, void*);
    static void handle_set_class(struct wl_listener*, void*);
    static void handle_set_role(struct wl_listener*, void*);
    static void handle_set_window_type(struct wl_listener*, void*);
    static void handle_set_hints(struct wl_listener*, void*);
    static void handle_set_decorations(struct wl_listener*, void*);
    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);
    static void handle_override_redirect(struct wl_listener*, void*);

    struct wlr_xwayland_surface* mp_wlr_xwayland_surface;

    struct wl_listener ml_commit;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
    struct wl_listener ml_request_maximize;
    struct wl_listener ml_request_minimize;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_request_activate;
    struct wl_listener ml_set_title;
    struct wl_listener ml_set_class;
    struct wl_listener ml_set_role;
    struct wl_listener ml_set_window_type;
    struct wl_listener ml_set_hints;
    struct wl_listener ml_set_decorations;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;
    struct wl_listener ml_override_redirect;

}* XWaylandView_ptr;

typedef struct XWaylandUnmanaged final {
    XWaylandUnmanaged(struct wlr_xwayland_surface*);
    ~XWaylandUnmanaged();

    Pos m_pos;

    struct wlr_xwayland_surface* mp_wlr_xwayland_surface;

    struct wl_listener ml_request_activate;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_commit;
    struct wl_listener ml_set_geometry;
    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_destroy;
    struct wl_listener ml_override_redirect;

}* XWaylandUnmanaged_ptr;
#endif
