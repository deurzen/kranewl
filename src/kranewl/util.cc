#include <kranewl/util.hh>

#define assert assert_
#include <spdlog/spdlog.h>
#undef assert

#include <utility>
extern "C" {
#include <unistd.h>
}

void
Util::die(const std::string&& msg)
{
    spdlog::critical(msg);
    exit(1);
}

void
Util::warn(const std::string&& msg)
{
    spdlog::warn(msg);
}

void
Util::assert(bool condition, std::string const&& msg)
{
    if (!condition)
        Util::die(std::forward<const std::string&&>(msg));
}
