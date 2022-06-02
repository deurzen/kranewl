#include <kranewl/env.hh>

extern "C" {
#include <sys/stat.h>
}

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <regex>

#include <spdlog/spdlog.h>

bool
file_exists(std::string const& path)
{
    struct stat buffer;
    if (!stat(path.c_str(), &buffer))
        return true;

    return false;
}

static inline std::string
evaluate_value(std::string const& value)
{
    static const std::regex regex(R"(\$([a-zA-Z0-9_]+))");

    std::string evaluation = {};
    std::smatch match;

    auto it = value.cbegin();
    auto end = value.cend();

    while (std::regex_search(it, end, match, regex)) {
        char const* env = getenv(match.str(1).c_str());
        evaluation += match.prefix();
        evaluation += env ? env : "";
        it = match.suffix().first;
    }

    evaluation.append(it, value.cend());
    return evaluation;
}

void
parse_and_set_env_vars(std::string const& env_path)
{
    if (!file_exists(env_path))
        return;

    struct Assignment_ctype : std::ctype<char> {
        Assignment_ctype()
            : std::ctype<char>(get_table())
        {}

        static mask const* get_table()
        {
            static mask rc[table_size];
            rc[static_cast<std::size_t>('=')] = std::ctype_base::space;
            rc[static_cast<std::size_t>('\n')] = std::ctype_base::space;
            rc[static_cast<std::size_t>('\r')] = std::ctype_base::space;
            return &rc[0];
        }
    };

    std::ifstream env_if(env_path);
    env_if.imbue(std::locale(env_if.getloc(), new Assignment_ctype));

    const auto isspace = [](char c) {
        return std::isspace(static_cast<unsigned char>(c));
    };

    std::string var, value;
    while (env_if >> var >> value) {
        var.erase(std::remove_if(var.begin(), var.end(), isspace), var.end());
        value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());

        spdlog::info("Setting environment variable: {}={}", var, value);
        setenv(var.c_str(), evaluate_value(value).c_str(), true);
    }
}
