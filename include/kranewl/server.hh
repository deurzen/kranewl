#pragma once

#include <kranewl/conf/state.hh>

#include <wayland-server.h>
#include <wayland-util.h>

#include <string>

class Server final
{
public:
    Server(std::string&& wm_name)
        : m_wm_name(wm_name) {}

    ~Server() {};

private:
    std::string m_wm_name;



};
