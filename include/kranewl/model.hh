#pragma once

#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
#include <kranewl/input/bindings.hh>
#include <kranewl/layout.hh>
#include <kranewl/placement.hh>
#include <kranewl/tree/view.hh>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

extern "C" {
#include <sys/types.h>
}

typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef class Server* Server_ptr;
typedef struct View* View_ptr;
typedef struct XDGView* XDGView_ptr;
#ifdef XWAYLAND
typedef struct XWaylandView* XWaylandView_ptr;
#endif
class Config;

class Model final
{
public:
    Model(Config const&, std::optional<std::string>);
    ~Model();

    void register_server(Server_ptr);
    void exit();

    KeyBindings const& key_bindings() const;

    Output_ptr create_output(struct wlr_output*, struct wlr_scene_output*, Region const&&);
    void register_output(Output_ptr);
    void unregister_output(Output_ptr);

    XDGView_ptr create_xdg_shell_view(struct wlr_xdg_surface*, Seat_ptr);
#ifdef XWAYLAND
    XWaylandView_ptr create_xwayland_view(struct wlr_xwayland_surface*, Seat_ptr);
#endif
    void register_view(View_ptr);
    void unregister_view(View_ptr);

    void map_view(View_ptr);
    void unmap_view(View_ptr);
    void iconify_view(View_ptr);
    void deiconify_view(View_ptr);
    void disown_view(View_ptr);
    void reclaim_view(View_ptr);
    void focus_view(View_ptr);
    void unfocus_view(View_ptr);
    void place_view(Placement&);

    void sync_focus();

    void move_view_to_workspace(View_ptr, Index);
    void move_view_to_workspace(View_ptr, Workspace_ptr);
    void move_view_to_context(View_ptr, Index);
    void move_view_to_context(View_ptr, Context_ptr);
    void move_view_to_output(View_ptr, Index);
    void move_view_to_output(View_ptr, Output_ptr);
    void move_view_to_focused_output(View_ptr);

    void activate_workspace(Index);
    void activate_workspace(Workspace_ptr);
    void activate_context(Index);
    void activate_context(Context_ptr);
    void activate_output(Index);
    void activate_output(Output_ptr);

    void toggle_layout();
    void set_layout(LayoutHandler::LayoutKind);
    void set_layout_retain_region(LayoutHandler::LayoutKind);

    void apply_layout(Index);
    void apply_layout(Workspace_ptr);

    bool is_free(View_ptr) const;

    void spawn_external(std::string&&) const;

private:
    Server_ptr mp_server;
    Config const& m_config;

    bool m_running;

    Cycle<Output_ptr> m_outputs;
    Cycle<Context_ptr> m_contexts;
    Cycle<Workspace_ptr> m_workspaces;

    Output_ptr mp_output;
    Output_ptr mp_fallback_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

    Output_ptr mp_prev_output;
    Context_ptr mp_prev_context;
    Workspace_ptr mp_prev_workspace;

    std::unordered_map<Uid, View_ptr> m_view_map;
    std::unordered_map<pid_t, View_ptr> m_pid_map;
    std::unordered_map<View_ptr, Region> m_fullscreen_map;

    std::vector<View_ptr> m_sticky_views;
    std::vector<View_ptr> m_unmanaged_views;

    View_ptr mp_focus;

    const KeyBindings m_key_bindings;
    const MouseBindings m_mouse_bindings;

};
