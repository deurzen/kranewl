#pragma once

#include <kranewl/input/bindings.hh>

#include <string>
#include <memory>

extern "C" {
#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
}

struct Config final {
    KeyBindings key_bindings;
    MouseBindings mouse_bindings;
};

class ConfigParser final {
public:
    ConfigParser(std::string const& config_path)
        : m_state(luaL_newstate()),
          m_config_path(config_path)
    {
        luaL_openlibs(m_state.get());
    }

    Config generate_config() const noexcept;

private:
    struct lua_State_deleter {
        void operator()(lua_State* L) const {
            lua_close(L);
        }
    };

    bool parse_decorations(Config&) const noexcept;
    bool parse_outputs(Config&)     const noexcept;
    bool parse_commands(Config&)    const noexcept;
    bool parse_bindings(Config&)    const noexcept;

    std::unique_ptr<lua_State, lua_State_deleter> m_state;
    std::string const& m_config_path;

};
