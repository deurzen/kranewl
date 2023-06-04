#pragma once

#include <kranewl/input/bindings.hh>

#include <string>
#include <memory>

struct Config final {
    KeyBindings key_bindings;
    CursorBindings cursor_bindings;
};

class ConfigParser final {
public:
    ConfigParser(std::string const& config_path)
        : m_config_path(config_path)
    {}

    Config generate_config() const noexcept;

private:
    std::string const& m_config_path;

};
