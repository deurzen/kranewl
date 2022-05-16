#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include <version.hh>

#include <kranewl/model.hh>
#include <kranewl/server.hh>
#include <kranewl/conf/options.hh>

#include <spdlog/spdlog.h>

#include <string>

int
main(int argc, char** argv)
{
    Options options = parse_options(argc, argv);

    spdlog::info("Initializing kranewl-" VERSION);

    Server server{"kranewl"};
    Model{server}.run();

    return EXIT_SUCCESS;
}
