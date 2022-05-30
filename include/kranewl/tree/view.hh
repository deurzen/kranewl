#pragma once

#include <kranewl/common.hh>
#include <kranewl/decoration.hh>
#include <kranewl/geometry.hh>
#include <kranewl/scene-layer.hh>

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

    enum class OutsideState {
        Focused,
        FocusedDisowned,
        FocusedSticky,
        Unfocused,
        UnfocusedDisowned,
        UnfocusedSticky,
        Urgent
    };

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
        struct wlr_surface*
    );
#ifdef XWAYLAND
    View(
        XWaylandView_ptr,
        Uid,
        Server_ptr,
        Model_ptr,
        Seat_ptr,
        struct wlr_surface*
    );
#endif

    virtual ~View();

    virtual Region constraints() = 0;
    virtual pid_t pid() = 0;
    virtual bool prefers_floating() = 0;
    virtual View_ptr is_transient_for() = 0;

    virtual void focus(Toggle) = 0;
    virtual void activate(Toggle) = 0;
    virtual void set_tiled(Toggle) = 0;
    virtual void set_fullscreen(Toggle) = 0;
    virtual void set_resizing(Toggle) = 0;

    virtual void configure(Region const&, Extents const&, bool) = 0;
    virtual void close() = 0;
    virtual void close_popups() = 0;
    virtual void destroy() = 0;

    void map();
    void unmap();
    void tile(Toggle);
    void relayer(SceneLayer);
    void raise() const;
    void lower() const;

    void render_decoration();

    bool activated() const { return m_activated; }
    bool focused() const { return m_focused; }
    bool mapped() const { return m_mapped; }
    bool managed() const { return m_managed; }
    bool urgent() const { return m_urgent; }
    bool floating() const { return m_floating; }
    bool fullscreen() const { return m_fullscreen; }
    bool scratchpad() const { return m_scratchpad; }
    bool contained() const { return m_contained; }
    bool invincible() const { return m_invincible; }
    bool sticky() const { return m_sticky; }
    bool iconifyable() const { return m_iconifyable; }
    bool iconified() const { return m_iconified; }
    bool disowned() const { return m_disowned; }
    void set_activated(bool);
    void set_focused(bool);
    void set_mapped(bool);
    void set_managed(bool);
    void set_urgent(bool);
    void set_floating(bool);
    void set_fullscreen(bool);
    void set_scratchpad(bool);
    void set_contained(bool);
    void set_invincible(bool);
    void set_sticky(bool);
    void set_iconifyable(bool);
    void set_iconified(bool);
    void set_disowned(bool);

    std::chrono::time_point<std::chrono::steady_clock> last_focused() const;
    std::chrono::time_point<std::chrono::steady_clock> last_touched() const;
    std::chrono::time_point<std::chrono::steady_clock> managed_since() const;

    uint32_t free_decoration_to_wlr_edges() const;
    uint32_t tile_decoration_to_wlr_edges() const;
    Region const& free_region() const { return m_free_region; }
    Pos const& free_pos() const { return m_free_region.pos; }
    Region const& tile_region() const { return m_tile_region; }
    Region const& active_region() const { return m_active_region; }
    Region const& prev_region() const { return m_prev_region; }
    void set_free_region(Region const&);
    void set_free_pos(Pos const&);
    void set_tile_region(Region const&);
    Dim const& minimum_dim() const { return m_minimum_dim; }
    Dim const& preferred_dim() const { return m_preferred_dim; }
    void set_minimum_dim(Dim const& minimum_dim) { m_minimum_dim = minimum_dim; }
    void set_preferred_dim(Dim const& preferred_dim) { m_preferred_dim = preferred_dim; }

    Decoration const& free_decoration() const { return m_free_decoration; }
    Decoration const& tile_decoration() const { return m_tile_decoration; }
    Decoration const& active_decoration() const { return m_active_decoration; }
    void set_free_decoration(Decoration const&);
    void set_tile_decoration(Decoration const&);

    void touch() { m_last_touched = std::chrono::steady_clock::now(); }
    void format_uid();

    static bool
    is_free(View_ptr view)
    {
        return (view->m_floating && (!view->m_fullscreen || view->m_contained))
            || view->m_disowned
            || !view->m_managed;
    }

    OutsideState outside_state() const;

    Uid m_uid;
    std::string m_uid_formatted;

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

protected:

    struct {
        struct wl_signal unmap;
    } m_events;

private:
    Decoration m_tile_decoration;
    Decoration m_free_decoration;
    Decoration m_active_decoration;

    Dim m_minimum_dim;
    Dim m_preferred_dim;
    Region m_free_region;
    Region m_tile_region;
    Region m_active_region;
    Region m_prev_region;
    Region m_inner_region;

    bool m_activated;
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

    SceneLayer m_scene_layer;

    OutsideState m_outside_state;

    std::chrono::time_point<std::chrono::steady_clock> m_last_focused;
    std::chrono::time_point<std::chrono::steady_clock> m_last_touched;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

    void set_inner_region(Region const&);
    void set_active_region(Region const&);
    void set_active_pos(Pos const&);

}* View_ptr;
