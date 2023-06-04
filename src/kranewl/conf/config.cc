#include <kranewl/conf/config.hh>

#include <spdlog/spdlog.h>

#include <linux/input-event-codes.h>
#include <wlr/types/wlr_keyboard.h>

Config
ConfigParser::generate_config() const noexcept
{
    Config config;
    spdlog::info("Generating config from " + m_config_path);

    return config;
}
