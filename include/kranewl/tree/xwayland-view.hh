#pragma once

#ifdef XWAYLAND
#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct XWayland* XWayland_ptr;

typedef struct XWaylandView final : public View {
    XWaylandView(
        struct wlr_xwayland_surface*,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        XWayland_ptr
    );

    ~XWaylandView();

    void format_uid() override;

    Region constraints() override;
    pid_t pid() override;
    bool prefers_floating() override;

    void focus(Toggle) override;
    void activate(Toggle) override;
    void set_fullscreen(Toggle) override;

    void configure(Region const&, Extents const&, bool) override;
    void close() override;
    void close_popups() override;

    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_commit(struct wl_listener*, void*);
    static void handle_request_activate(struct wl_listener*, void*);
    static void handle_request_configure(struct wl_listener*, void*);
    static void handle_request_fullscreen(struct wl_listener*, void*);
    static void handle_request_minimize(struct wl_listener*, void*);
    static void handle_request_maximize(struct wl_listener*, void*);
    static void handle_request_move(struct wl_listener*, void*);
    static void handle_request_resize(struct wl_listener*, void*);
    static void handle_set_override_redirect(struct wl_listener*, void*);
    static void handle_set_title(struct wl_listener*, void*);
    static void handle_set_class(struct wl_listener*, void*);
    static void handle_set_hints(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    XWayland_ptr mp_xwayland;

    std::string m_class;
    std::string m_instance;

    struct wlr_xwayland_surface* mp_wlr_xwayland_surface;

    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_commit;
    struct wl_listener ml_request_activate;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_request_minimize;
    struct wl_listener ml_request_maximize;
    struct wl_listener ml_request_move;
    struct wl_listener ml_request_resize;
    struct wl_listener ml_set_override_redirect;
    struct wl_listener ml_set_title;
    struct wl_listener ml_set_class;
    struct wl_listener ml_set_hints;
    struct wl_listener ml_destroy;

}* XWaylandView_ptr;

typedef struct XWaylandUnmanaged final : public Node {
    XWaylandUnmanaged(
        struct wlr_xwayland_surface*,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        XWayland_ptr
    );

    ~XWaylandUnmanaged();

    void format_uid() override;

    pid_t pid();

    static void handle_map(struct wl_listener*, void*);
    static void handle_unmap(struct wl_listener*, void*);
    static void handle_commit(struct wl_listener*, void*);
    static void handle_set_override_redirect(struct wl_listener*, void*);
    static void handle_set_geometry(struct wl_listener*, void*);
    static void handle_request_activate(struct wl_listener*, void*);
    static void handle_request_configure(struct wl_listener*, void*);
    static void handle_request_fullscreen(struct wl_listener*, void*);
    static void handle_destroy(struct wl_listener*, void*);

    Server_ptr mp_server;
    Model_ptr mp_model;
    Seat_ptr mp_seat;

    Output_ptr mp_output;

    XWayland_ptr mp_xwayland;

    Region m_region;

    std::string m_title;
    std::string m_title_formatted;
    std::string m_app_id;
    std::string m_class;
    std::string m_instance;

    struct wlr_xwayland_surface* mp_wlr_xwayland_surface;

    struct wlr_surface* mp_wlr_surface;
    struct wlr_scene_node* mp_scene;
    struct wlr_scene_node* mp_scene_surface;

    pid_t m_pid;

    struct wl_listener ml_map;
    struct wl_listener ml_unmap;
    struct wl_listener ml_commit;
    struct wl_listener ml_set_override_redirect;
    struct wl_listener ml_set_geometry;
    struct wl_listener ml_request_activate;
    struct wl_listener ml_request_configure;
    struct wl_listener ml_request_fullscreen;
    struct wl_listener ml_destroy;

}* XWaylandUnmanaged_ptr;
#endif
