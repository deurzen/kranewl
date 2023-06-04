#pragma once

#ifdef XWAYLAND
#include <kranewl/tree/view.hh>
#include <kranewl/util.hh>

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct XWayland* XWayland_ptr;

typedef struct XWaylandView final : public View {
    XWaylandView(
        struct wlr_xwayland_surface*,
        Server_ptr,
        Manager_ptr,
        Seat_ptr,
        XWayland_ptr
    );

    ~XWaylandView();

    void format_uid() override;

    std::string const& class_() const { return m_class; }
    std::string const& instance() const { return m_instance; }
    void set_class(std::string const& class_) { m_class = class_; }
    void set_instance(std::string const& instance) { m_instance = instance; }

    Region constraints() override;
    pid_t retrieve_pid() override;
    bool prefers_floating() override;

    void focus(Toggle) override;
    void activate(Toggle) override;
    void effectuate_fullscreen(bool) override;

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

private:
    std::string m_class;
    std::string m_instance;

}* XWaylandView_ptr;

#endif
