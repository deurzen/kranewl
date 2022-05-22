#pragma once

#include <kranewl/bindings.hh>
#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
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

    Output_ptr create_output(struct wlr_output*, struct wlr_scene_output*);
    void register_output(Output_ptr);
    void unregister_output(Output_ptr);

    XDGView_ptr create_xdg_shell_view(struct wlr_xdg_surface*, Seat_ptr);
#ifdef XWAYLAND
    XWaylandView_ptr create_xwayland_view(struct wlr_xwayland_surface*, Seat_ptr);
#endif
    void register_view(View_ptr);
    void unregister_view(View_ptr);

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

    KeyBindings m_key_bindings;
    MouseBindings m_mouse_bindings;

};
