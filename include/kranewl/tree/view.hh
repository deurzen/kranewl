#pragma once

#include <kranewl/common.hh>
#include <kranewl/decoration.hh>
#include <kranewl/geometry.hh>
#include <kranewl/scene-layer.hh>
#include <kranewl/tree/node.hh>

#include <vector>
#include <chrono>

extern "C" {
#include <sys/types.h>
#include <wayland-server-core.h>
}

typedef class Server* Server_ptr;
typedef class Manager* Manager_ptr;
typedef class Seat* Seat_ptr;
typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef struct XDGView* XDGView_ptr;
#ifdef XWAYLAND
typedef struct XWaylandView* XWaylandView_ptr;
#endif
typedef struct View* View_ptr;

typedef struct View : public Node {
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

    View(
        XDGView_ptr,
        Uid,
        Server_ptr,
        Manager_ptr,
        Seat_ptr,
        struct wlr_surface*
    );
#ifdef XWAYLAND
    View(
        XWaylandView_ptr,
        Uid,
        Server_ptr,
        Manager_ptr,
        Seat_ptr,
        struct wlr_surface*
    );
#endif

    virtual ~View();

    void format_uid() override;

    virtual Region constraints() = 0;
    virtual pid_t retrieve_pid() = 0;
    virtual bool prefers_floating() = 0;

    virtual void focus(Toggle) = 0;
    virtual void activate(Toggle) = 0;
    virtual void effectuate_fullscreen(bool) = 0;

    virtual void configure(Region const&, Extents const&, bool) = 0;
    virtual void close() = 0;
    virtual void close_popups() = 0;

    void map();
    void unmap();
    void center();
    void tile(Toggle);
    void relayer(SceneLayer);
    void raise() const;
    void lower() const;

    void render_decoration();
    void render_cycle_indicator();

    std::string const& title() const { return m_title; }
    std::string const& title_formatted() const { return m_title_formatted; }
    std::string const& app_id() const { return m_app_id; }
    std::string const& handle() const { return m_handle; }
    void set_title(std::string const& title) { m_title = title; }
    void set_title_formatted(std::string const& title_formatted) { m_title_formatted = title_formatted; }
    void set_app_id(std::string const& app_id) { m_app_id = app_id; }
    void set_handle(std::string const& handle) { m_handle = handle; }


    pid_t pid() const { return m_pid; }
    void set_pid(pid_t pid) { m_pid = pid; }

    bool activated() const { return m_activated; }
    bool focused() const { return m_focused; }
    bool mapped() const { return m_mapped; }
    bool managed() const { return m_managed; }
    bool free() const { return m_free; }
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
    void set_free(bool);
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

    bool belongs_to_active_track() const;

    std::chrono::time_point<std::chrono::steady_clock> last_focused() const;
    std::chrono::time_point<std::chrono::steady_clock> last_touched() const;
    std::chrono::time_point<std::chrono::steady_clock> managed_since() const;

    uint32_t free_decoration_to_wlr_edges() const;
    uint32_t tile_decoration_to_wlr_edges() const;
    Region const& free_region() const { return m_free_region; }
    Pos const& free_pos() const { return m_free_region.pos; }
    Region const& tile_region() const { return m_tile_region; }
    Region const& active_region() const { return m_active_region; }
    Region const& inner_region() const { return m_inner_region; }
    Region const& prev_region() const { return m_prev_region; }
    void set_free_region(Region const&);
    void set_free_pos(Pos const&);
    void set_tile_region(Region const&);
    Dim const& minimum_dim() const { return m_minimum_dim; }
    Dim const& preferred_dim() const { return m_preferred_dim; }
    void set_minimum_dim(Dim const& minimum_dim) { m_minimum_dim = minimum_dim; }
    void set_preferred_dim(Dim const& preferred_dim) { m_preferred_dim = preferred_dim; }
    std::optional<Pos> const& last_cursor_pos() const { return m_last_cursor_pos; }
    void set_last_cursor_pos(std::optional<Pos> const& last_cursor_pos) { m_last_cursor_pos = last_cursor_pos; }

    Decoration const& free_decoration() const { return m_free_decoration; }
    Decoration const& tile_decoration() const { return m_tile_decoration; }
    Decoration const& active_decoration() const { return m_active_decoration; }
    void set_free_decoration(Decoration const&);
    void set_tile_decoration(Decoration const&);

    void indicate_as_next();
    void unindicate_as_next();
    void indicate_as_prev();
    void unindicate_as_prev();

    void touch() { m_last_touched = std::chrono::steady_clock::now(); }

    static bool
    is_free(View_ptr view)
    {
        return (view->m_floating && (!view->m_fullscreen || view->m_contained))
            || view->m_disowned;
    }

    SceneLayer scene_layer() const { return m_scene_layer; }
    OutsideState outside_state() const;

    Server_ptr mp_server;
    Manager_ptr mp_manager;
    Seat_ptr mp_seat;

    Output_ptr mp_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

    struct wlr_surface* mp_wlr_surface;
    struct wlr_scene_tree* mp_scene;
    struct wlr_scene_tree* mp_scene_surface;
    struct wlr_scene_rect* m_protrusions[4]; // top, bottom, left, right
    struct wlr_scene_rect* m_next_indicator[2]; // horizontal portion, vertical portion
    struct wlr_scene_rect* m_prev_indicator[2]; // horizontal portion, vertical portion

    float m_alpha;
    uint32_t m_resize;

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
    std::optional<Pos> m_last_cursor_pos;

    bool m_activated;
    bool m_focused;
    bool m_mapped;
    bool m_managed;
    bool m_free;
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

    std::string m_title;
    std::string m_title_formatted;
    std::string m_app_id;
    std::string m_handle;

    pid_t m_pid;

    SceneLayer m_scene_layer;

    OutsideState m_outside_state;

    std::chrono::time_point<std::chrono::steady_clock> m_last_focused;
    std::chrono::time_point<std::chrono::steady_clock> m_last_touched;
    std::chrono::time_point<std::chrono::steady_clock> m_managed_since;

    void set_inner_region(Region const&);
    void set_active_region(Region const&);
    void set_active_pos(Pos const&);

}* View_ptr;
