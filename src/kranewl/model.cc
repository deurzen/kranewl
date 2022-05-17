#include <kranewl/model.hh>

#include <kranewl/conf/config.hh>
#include <kranewl/exec.hh>
#include <kranewl/server.hh>

#include <spdlog/spdlog.h>

Model::Model(
    Server& server,
    Config const& config,
    [[maybe_unused]] std::optional<std::string> autostart_path
)
    : m_server(server),
      m_config(config)
{
#ifdef NDEBUG
    if (autostart_path) {
        spdlog::info("Executing autostart file at " + *autostart_path);
        exec_external(*autostart_path);
    }
#endif

    m_server.start();
}

Model::~Model()
{}

void
Model::run()
{
    // TODO
}
