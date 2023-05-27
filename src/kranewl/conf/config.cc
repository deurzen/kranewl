#include <kranewl/conf/config.hh>

#include <spdlog/spdlog.h>

extern "C" {
/* #include <lua5.4/lua.h> */
/* #include <lua5.4/lauxlib.h> */
/* #include <lua5.4/lualib.h> */
}

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_keyboard.h>

/* static void */
/* print_table(lua_State* L) */
/* { */
/*     lua_pushnil(L); */

/*     while(lua_next(L, -2) != 0) */
/*     { */
/*         if(lua_isstring(L, -1)) */
/*             printf("%s = %s\n", lua_tostring(L, -2), lua_tostring(L, -1)); */
/*         else if(lua_isnumber(L, -1)) */
/*             printf("%s = %f\n", lua_tostring(L, -2), lua_tonumber(L, -1)); */
/*         else if(lua_istable(L, -1)) { */
/*             printf("%s:\n", lua_tostring(L, -2)); */
/*             print_table(L); */
/*         } */

/*         lua_pop(L, 1); */
/*     } */
/* } */

Config
ConfigParser::generate_config() const noexcept
{
    /* Config config; */
    /* spdlog::info("Generating config from " + m_config_path); */

    /* if (luaL_dofile(m_state.get(), m_config_path.c_str()) != LUA_OK) { */
    /*     std::string error_msg = lua_tostring(m_state.get(), -1); */
    /*     spdlog::error("Error generating config: " + error_msg); */
    /*     spdlog::warn("Falling back to default configuration"); */
    /*     return config; */
    /* } */

    /* parse_decorations(config); */
    /* parse_outputs(config); */
    /* parse_commands(config); */
    /* parse_bindings(config); */

    /* return config; */
}

bool
ConfigParser::parse_decorations(Config& config) const noexcept
{
    /* // TODO */
    /* static_cast<void>(config); */

    /* lua_getglobal(m_state.get(), "decorations"); */

    /* return true; */
}

bool
ConfigParser::parse_outputs(Config& config) const noexcept
{
    /* // TODO */
    /* static_cast<void>(config); */

    /* lua_getglobal(m_state.get(), "outputs"); */

    /* return true; */
}

bool
ConfigParser::parse_commands(Config& config) const noexcept
{
    /* // TODO */
    /* static_cast<void>(config); */

    /* lua_getglobal(m_state.get(), "commands"); */

    /* return true; */
}

bool
ConfigParser::parse_bindings(Config& config) const noexcept
{
    /* // TODO */
    /* static_cast<void>(config); */

    /* lua_getglobal(m_state.get(), "bindings"); */

    /* return true; */
}
