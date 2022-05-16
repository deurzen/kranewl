#pragma once

#include <kranewl/server.hh>

class Model final
{
public:
    Model(Server& server)
        : m_server(server)
    {}

    ~Model() {}

    void run();

private:
    Server& m_server;

};
