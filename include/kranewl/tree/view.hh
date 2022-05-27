#pragma once

#include <kranewl/common.hh>
#include <kranewl/decoration.hh>
#include <kranewl/geometry.hh>
#include <kranewl/model.hh>
#include <kranewl/tree/surface.hh>

#include <vector>
#include <chrono>

extern "C" {
#include <sys/types.h>
#include <wayland-server-core.h>
}

typedef class Server* Server_ptr;
typedef class Model* Model_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct XDGView* XDGView_ptr;
#ifdef XWAYLAND
typedef struct XWaylandView* XWaylandView_ptr;
#endif
typedef struct View* View_ptr;

typedef struct View {
    static constexpr Dim MIN_VIEW_DIM = Dim{25, 10};
    static constexpr Dim PREFERRED_INIT_VIEW_DIM = Dim{480, 260};

    enum class Type {
        XDGShell,
#ifdef XWAYLAND
        XWayland,
#endif
    };

    View(
        XDGView_ptr,
        Uid,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        struct wlr_surface*,
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*)
    );
#ifdef XWAYLAND
    View(
        XWaylandView_ptr,
        Uid,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        struct wlr_surface*,
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*),
        void(*)(wl_listener*, void*)
    );
#endif

    virtual ~View();

    void map(struct wlr_surface*, bool, struct wlr_output*, bool);
    void unmap();

    void touch();

    virtual void focus(bool) = 0;
    virtual void moveresize(Region const&, Extents const&, bool) = 0;
    virtual void kill() = 0;

    static bool
    is_free(View_ptr view)
    {
        return (view->m_floating && (!view->m_fullscreen || view->m_contained))
            || !view->m_managed
            || view->m_disowned;
    }

    uint32_t free_decoration_to_wlr_edges() const;
    uint32_t tile_decoration_to_wlr_edges() const;

    void set_free_region(Region const&);
    void set_tile_region(Region const&);

    void set_free_decoration(Decoration const&);
    void set_tile_decoration(Decoration const&);

    Uid m_uid;
    Type m_type;

    Server_ptr mp_server;
    Model_ptr mp_model;
    Seat_ptr mp_seat;

    Output_ptr mp_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

    struct wlr_surface* mp_wlr_surface;
    struct wlr_scene_node* mp_scene;
    struct wlr_scene_node* mp_scene_surface;
    struct wlr_scene_rect* m_protrusions[4]; // top, bottom, left, right

    std::string m_title;
    std::string m_title_formatted;
    std::string m_app_id;

    float m_alpha;
    uint32_t m_resize;

    pid_t m_pid;

    Decoration m_tile_decoration;
    Decoration m_free_decoration;
    Decoration m_active_decoration;

    Dim m_minimum_dim;
    Dim m_preferred_dim;
    Region m_free_region;
    Region m_tile_region;
    Region m_active_region;
    Region m_previous_region;
    Region m_inner_region;

    bool m_focused;
    bool m_mapped;
    bool m_managed;
    bool m_urgent;
    bool m_floating;
    bool m_fullscreen;
    bool m_scratchpad;
    bool m_contained;
    bool m_invincible;
    bool m_sticky;
    bool m_iconifyable;
    bool m_iconified;
    bool m_disowned;

    std::chrono::time_point<std::chrono::steady_clock> m_last_focused;
    std::chrono::time_point<std::chrono::steady_clock> m_last_touched;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

protected:
    void set_inner_region(Region const&);
    void set_active_region(Region const&);

    struct wlr_foreign_toplevel_handle_v1* foreign_toplevel;

    struct wl_listener ml_foreign_activate_request;
    struct wl_listener ml_foreign_fullscreen_request;
    struct wl_listener ml_foreign_close_request;
    struct wl_listener ml_foreign_destroy;
    struct wl_listener ml_surface_new_subsurface;

    struct {
        struct wl_signal unmap;
    } m_events;

}* View_ptr;

typedef struct ViewChild* ViewChild_ptr;
typedef struct SubsurfaceViewChild* SubsurfaceViewChild_ptr;
typedef struct PopupViewChild* PopupViewChild_ptr;

typedef struct ViewChild {
    enum class Type {
        Subsurface,
        Popup,
    };

protected:
    ViewChild(SubsurfaceViewChild_ptr);
    ViewChild(PopupViewChild_ptr);

    virtual ~ViewChild();

public:
    Uid m_uid;
    Type m_type;

    View_ptr mp_view;
    ViewChild_ptr mp_parent;
    std::vector<ViewChild_ptr> m_children;

    struct wlr_scene_node* mp_scene;
    struct wlr_scene_node* mp_scene_surface;

    bool m_mapped;

    float m_alpha;
    uint32_t m_resize;

    pid_t m_pid;

    struct wl_listener ml_surface_commit;
    struct wl_listener ml_surface_new_subsurface;
    struct wl_listener ml_surface_map;
    struct wl_listener ml_surface_unmap;
    struct wl_listener ml_surface_destroy;
    struct wl_listener ml_view_unmap;

}* ViewChild_ptr;
