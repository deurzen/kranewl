#pragma once

#include <kranewl/bindings.hh>
#include <kranewl/common.hh>
#include <kranewl/cycle.hh>
#include <kranewl/geometry.hh>
#include <kranewl/tree/client.hh>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

typedef class Output* Output_ptr;
typedef class Context* Context_ptr;
typedef class Workspace* Workspace_ptr;
typedef class Server* Server_ptr;
class Config;
class Model final
{
public:
    Model(Config const&, std::optional<std::string>);
    ~Model();

    void register_server(Server_ptr);
    void exit();

    Keyboard_ptr create_keyboard(struct wlr_output*, struct wlr_scene_output*);
    void register_keyboard(Keyboard_ptr);
    void unregister_keyboard(Keyboard_ptr);

    Output_ptr create_output(struct wlr_output*, struct wlr_scene_output*);
    void register_output(Output_ptr);
    void unregister_output(Output_ptr);

    Client_ptr create_client(Surface);
    void register_client(Client_ptr);
    void unregister_client(Client_ptr);

    void spawn_external(std::string&&) const;

private:
    Server_ptr mp_server;
    Config const& m_config;

    bool m_running;

    Cycle<Output_ptr> m_outputs;
    Cycle<Keyboard_ptr> m_keyboards;
    Cycle<Context_ptr> m_contexts;
    Cycle<Workspace_ptr> m_workspaces;

    Output_ptr mp_output;
    Output_ptr mp_fallback_output;
    Context_ptr mp_context;
    Workspace_ptr mp_workspace;

    Output_ptr mp_prev_output;
    Context_ptr mp_prev_context;
    Workspace_ptr mp_prev_workspace;

    std::unordered_map<Uid, Client_ptr> m_client_map;
    std::unordered_map<Pid, Client_ptr> m_pid_map;
    std::unordered_map<Client_ptr, Region> m_fullscreen_map;

    std::vector<Client_ptr> m_sticky_clients;
    std::vector<Client_ptr> m_unmanaged_clients;

    Client_ptr mp_focus;

    KeyBindings m_key_bindings;
    MouseBindings m_mouse_bindings;

};
