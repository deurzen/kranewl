#include <kranewl/conf/config.hh>

#include <spdlog/spdlog.h>

extern "C" {
#include <lua5.4/lua.h>
#include <lua5.4/lauxlib.h>
#include <lua5.4/lualib.h>
}

Config
ConfigParser::generate_config() const noexcept
{
    Config config;
    spdlog::info("Generating config from " + m_config_path);

    if (luaL_dofile(m_state.get(), m_config_path.c_str()) != LUA_OK) {
        std::string error_msg = lua_tostring(m_state.get(), -1);
        spdlog::error("Error generating config: " + error_msg);
        spdlog::warn("Falling back to default configuration");
        return config;
    }

    parse_decorations(config);
    parse_outputs(config);
    parse_commands(config);
    parse_bindings(config);

    return config;
}

bool
ConfigParser::parse_decorations(Config& config) const noexcept
{
    static_cast<void>(config);

    lua_getglobal(m_state.get(), "decorations");
    if (lua_istable(m_state.get(), -1))
        ; // TODO

    return true;
}

bool
ConfigParser::parse_outputs(Config& config) const noexcept
{
    static_cast<void>(config);

    lua_getglobal(m_state.get(), "outputs");
    if (lua_istable(m_state.get(), -1))
        ; // TODO

    return true;
}

bool
ConfigParser::parse_commands(Config& config) const noexcept
{
    static_cast<void>(config);

    lua_getglobal(m_state.get(), "commands");
    if (lua_istable(m_state.get(), -1))
        ; // TODO

    return true;
}

bool
ConfigParser::parse_bindings(Config& config) const noexcept
{
    static_cast<void>(config);

    lua_getglobal(m_state.get(), "bindings");
    if (lua_istable(m_state.get(), -1))
        ; // TODO

    return true;
}
