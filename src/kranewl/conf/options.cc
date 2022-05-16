#include <version.hh>

#include <kranewl/conf/options.hh>

#include <iostream>
#include <unistd.h>

const std::string usage = "Usage: kranewl [...options]\n\n"
    "Options: \n"
    "  -x <autostart_file> Path to an executable autostart file.\n"
    "  -c <config_file>    Path to a configuration file.\n"
    "  -h                  Prints this message.\n"
    "  -v                  Prints the version.";

Options
parse_options(int argc, char **argv)
{
    Options options;
    int opt;

    while ((opt = getopt(argc, argv, "vhx:c:")) != -1) {
        switch (opt) {
        case 'x':
            options.autostart = optarg;
            break;

        case 'c':
            options.config_path = optarg;
            break;

        case 'v':
            std::cout << VERSION << std::endl << std::flush;
            exit(EXIT_SUCCESS);
            break;

        case '?':
        case 'h':
        default:
            std::cout << usage << std::endl << std::flush;
            exit(EXIT_SUCCESS);
            break;
        }
    }

    return options;
}
