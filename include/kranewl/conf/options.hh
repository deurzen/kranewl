#pragma once

#include <string>

struct Options {
    std::string config_path;
    std::string autostart;
};

Options parse_options(int, char **);
