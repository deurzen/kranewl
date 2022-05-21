#include <trace.hh>

#include <kranewl/model.hh>

#include <kranewl/common.hh>
#include <kranewl/conf/config.hh>
#include <kranewl/context.hh>
#include <kranewl/context.hh>
#include <kranewl/cycle.t.hh>
#include <kranewl/exec.hh>
#include <kranewl/input/keyboard.hh>
#include <kranewl/input/mouse.hh>
#include <kranewl/server.hh>
#include <kranewl/tree/output.hh>
#include <kranewl/workspace.hh>

#include <spdlog/spdlog.h>

Model::Model(
    Config const& config,
    [[maybe_unused]] std::optional<std::string> autostart_path
)
    : m_config{config},
      m_running{true},
      m_outputs{{}, true},
      m_contexts{{}, true},
      m_workspaces{{}, true},
      mp_output{nullptr},
      mp_context{nullptr},
      mp_workspace{nullptr},
      mp_prev_output{nullptr},
      mp_prev_context{nullptr},
      mp_prev_workspace{nullptr},
      m_client_map{},
      m_pid_map{},
      m_fullscreen_map{},
      m_sticky_clients{},
      m_unmanaged_clients{},
      mp_focus(nullptr),
      m_key_bindings{},
      m_mouse_bindings{}
{
    TRACE();

#ifdef NDEBUG
    if (autostart_path) {
        spdlog::info("Executing autostart file at " + *autostart_path);
        exec_external(*autostart_path);
    }
#endif

    static const std::vector<std::string> context_names{
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j"
    };

    static const std::vector<std::string> workspace_names{
        "main", "web", "term", {}, {}, {}, {}, {}, {}, {}
    };

    for (std::size_t i = 0; i < context_names.size(); ++i) {
        Context_ptr context = new Context(i, context_names[i]);
        m_contexts.insert_at_back(context);

        for (std::size_t j = 0; j < workspace_names.size(); ++j) {
            Workspace_ptr workspace = new Workspace(
                workspace_names.size() * i + j,
                workspace_names[j],
                context
            );

            m_workspaces.insert_at_back(workspace);
            context->register_workspace(workspace);
        }

        context->activate_workspace(Index{0});
    }

    m_contexts.activate_at_index(0);
    m_workspaces.activate_at_index(0);

    mp_context = *m_contexts.active_element();
    mp_workspace = *m_workspaces.active_element();
}

Model::~Model()
{}

void
Model::register_server(Server_ptr server)
{
    TRACE();

    mp_server = server;
}

Output_ptr
Model::create_output(
    Server_ptr server,
    struct wlr_output* wlr_output,
    struct wlr_scene_output* wlr_scene_output
)
{
    TRACE();

    Output_ptr output = new Output(
        mp_server,
        wlr_output,
        wlr_scene_output
    );

    register_output(output);

    return output;
}

void
Model::register_output(Output_ptr output)
{
    TRACE();

    m_outputs.insert_at_back(output);
}

void
Model::unregister_output(Output_ptr output)
{
    TRACE();

    m_outputs.remove_element(output);
}

Client_ptr
Model::create_client(Surface surface)
{
    Client_ptr client = new Client(
        mp_server,
        surface,
        mp_output,
        mp_context,
        mp_workspace
    );

    register_client(client);

    return client;
}

void
Model::register_client(Client_ptr client)
{
}

void
Model::unregister_client(Client_ptr client)
{}
