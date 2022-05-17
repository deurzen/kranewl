#pragma once

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

private:
    Server& m_server;
    Config const& m_config;

};
