#include <trace.hh>
#include <version.hh>

#include <kranewl/conf/config.hh>
#include <kranewl/conf/options.hh>
#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/decoration.hh>

#include <spdlog/spdlog.h>

extern "C" {
#include <unistd.h>
#include <wlr/util/log.h>
}

#include <string>

static inline bool
drop_privileges()
{
    if (getuid() != geteuid() || getgid() != getegid())
        if (setuid(getuid()) || setgid(getgid()))
            return false;

    if (!geteuid() || !getegid()) {
        spdlog::error("Running as root is prohibited");
        return false;
    }

    return true;
}

int
main(int argc, char** argv)
{
#ifndef NDEBUG
    wlr_log_init(WLR_DEBUG, nullptr);
#ifdef TRACING_ENABLED
    spdlog::set_level(spdlog::level::trace);
#else
    spdlog::set_level(spdlog::level::debug);
#endif
#else
    spdlog::set_level(spdlog::level::info);
#endif

    if (!drop_privileges()) {
        spdlog::critical("Could not drop privileges");
        return EXIT_FAILURE;
    }

    const Options options = parse_options(argc, argv);

    spdlog::info("Initializing kranewl-" VERSION);
    const ConfigParser config_parser{options.config_path};
    const Config config = config_parser.generate_config();

    Model model{config};
    Server server{&model};

    model.initialize(
        options.env_path,
        options.autostart_path
    );

    server.run();
    return EXIT_SUCCESS;
}
