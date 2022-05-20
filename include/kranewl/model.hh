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
class Server;
class Config;
class Model final
{
public:
    Model(Server&, Config const&, std::optional<std::string>);
    ~Model();

    void run();
    void exit();

    Output_ptr create_output(Surface);
    void register_output(Output_ptr);
    void unregister_output(Output_ptr);

    Client_ptr create_client(Surface);
    void register_client(Client_ptr);
    void unregister_client(Client_ptr);

    void spawn_external(std::string&&) const;

private:
    Server& m_server;
    Config const& m_config;

    bool m_running;

    Cycle<Output_ptr> m_outputs;
    Cycle<Context_ptr> m_contexts;
    Cycle<Workspace_ptr> m_workspaces;

    Output_ptr mp_output;
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
