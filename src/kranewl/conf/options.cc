#include <version.hh>

#include <kranewl/conf/options.hh>

#include <cassert>
#include <optional>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

static const std::string CONFIG_FILE = "kranewlrc.lua";
static const std::string DEFAULT_CONFIG = "/etc/kranewl/" + CONFIG_FILE;
static const std::string USAGE = "usage: kranewl [...options]\n\n"
    "options: \n"
    "  -a <autostart_file> Path to an executable autostart file.\n"
    "  -c <config_file>    Path to a configuration file.\n"
    "  -v                  Prints the version.\n"
    "  -h                  Prints this message.";

static std::string
default_user_path(std::string const& path) noexcept
{
    if (const char* prefix = std::getenv("XDG_CONFIG_HOME"))
        return std::string{prefix}
            + std::string{"/kranewl/"}
            + path;
    else
        return std::string{std::getenv("HOME")}
            + std::string{"/.config/kranewl/"}
            + path;
}

static bool
assert_permissions(std::string const& path, int mode) noexcept
{
    struct stat s;
    return stat(path.c_str(), &s) == 0
        && S_ISREG(s.st_mode)
        && !access(path.c_str(), mode);
}

static std::string const&
resolve_config_path(std::string& path) noexcept
{
    if (!path.empty())
        if (assert_permissions(path, R_OK))
            return path;

    path.assign(default_user_path(CONFIG_FILE));
    if (assert_permissions(path, R_OK))
        return path;

    return DEFAULT_CONFIG;
}

static std::optional<std::string>
resolve_autostart_path(std::string& path) noexcept
{
    const int mode = R_OK | X_OK;

    if (!path.empty())
        if (assert_permissions(path, mode))
            return {path};

    path.assign(default_user_path("autostart"));
    if (assert_permissions(path, mode))
        return {path};

    return {};
}

Options
parse_options(int argc, char** argv) noexcept
{
    std::string autostart_path, config_path;
    int opt;

    while ((opt = getopt(argc, argv, "vha:c:")) != -1) {
        switch (opt) {
        case 'a':
            autostart_path = optarg;
            break;

        case 'c':
            config_path = optarg;
            break;

        case 'v':
            std::cout << VERSION << std::endl << std::flush;
            std::exit(EXIT_SUCCESS);
            break;

        case '?':
        case 'h':
        default:
            std::cout << USAGE << std::endl << std::flush;
            std::exit(EXIT_SUCCESS);
            break;
        }
    }

    return Options(
        std::move(resolve_config_path(config_path)),
        resolve_autostart_path(autostart_path)
    );
}
