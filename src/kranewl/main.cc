#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <version.hh>

#include <kranewl/conf/config.hh>
#include <kranewl/conf/options.hh>
#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/decoration.hh>

#include <spdlog/spdlog.h>

extern "C" {
#include <wlr/util/log.h>
}

#include <string>

int
main(int argc, char** argv)
{
#ifndef NDEBUG
    /* wlr_log_init(WLR_DEBUG, NULL); */
#endif

    const Options options = parse_options(argc, argv);

    spdlog::info("Initializing kranewl-" VERSION);
    const ConfigParser config_parser{options.config_path};
    const Config config = config_parser.generate_config();

    Server server;
    Model{server, config, options.autostart_path}.run();

    return EXIT_SUCCESS;
}
