#pragma once

#include <optional>
#include <string>

struct Options final {
    Options(
        std::string const&& config_path_,
        std::string const&& env_path_,
        std::optional<std::string> autostart_path_
    )
        : config_path(config_path_),
          env_path(env_path_),
          autostart_path(autostart_path_)
    {}

    std::string config_path;
    std::string env_path;
    std::optional<std::string> autostart_path;
};

Options parse_options(int, char**) noexcept;
