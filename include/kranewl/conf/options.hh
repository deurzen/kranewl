#pragma once

#include <optional>
#include <string>

struct Options final {
    Options(
        std::string const&& _config_path,
        std::optional<std::string> _autostart_path
    )
        : config_path(_config_path),
          autostart_path(_autostart_path)
    {}

    std::string config_path;
    std::optional<std::string> autostart_path;
};

Options parse_options(int, char**) noexcept;
