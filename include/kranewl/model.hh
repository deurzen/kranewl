#pragma once

#include <kranewl/conf/config.hh>
#include <kranewl/exec.hh>
#include <kranewl/server.hh>

#include <spdlog/spdlog.h>

#include <optional>
#include <string>

class Model final
{
public:
    Model(
        Server& server,
        Config const& config,
        [[maybe_unused]] std::optional<std::string> autostart
    )
        : m_server(server),
          m_config(config)
    {
#ifdef NDEBUG
        if (autostart) {
            spdlog::info("Executing autostart at " + config_path);
            exec_external(*autostart);
        }
#endif
    }

    ~Model() {}

    void run();

private:
    Server& m_server;
    Config const& m_config;

};
