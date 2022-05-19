#pragma once

#include <kranewl/cycle.hh>

#include <optional>
#include <string>

class Server;
class Config;
class Model final
{
public:
    Model(Server&, Config const&, std::optional<std::string>);
    ~Model();

    void run();
    void exit();

    void spawn_external(std::string&&) const;

private:
    Server& m_server;
    Config const& m_config;

    /* Cycle<Partition_ptr> m_partitions; */
    /* Cycle<Context_ptr> m_contexts; */
    /* Cycle<Workspace_ptr> m_workspaces; */

};
