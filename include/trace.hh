#pragma once

#include <spdlog/spdlog.h>

#include <string>

#ifndef NDEBUG
#ifndef TRACING_DISABLED
#define TRACING_ENABLED 1
#endif
#endif

#ifdef TRACING_ENABLED
namespace tracing
{
    class Tracer {
    public:
        Tracer() = delete;
        Tracer(Tracer const&) = delete;
        Tracer(Tracer&&) = delete;

        Tracer& operator=(Tracer const&) = delete;
        Tracer& operator=(Tracer&&) = delete;

        Tracer(std::string const& fun, std::string const& file, int const line)
            : m_function_name{fun},
              m_file_name{file},
              m_line_number{line}
        {
            spdlog::trace("Entering function: "
                + m_function_name
                + " ("
                + m_file_name
                + ":"
                + std::to_string(m_line_number)
                + ")");
        }

        ~Tracer()
        {
            spdlog::trace("Leaving function: " + m_function_name);
        }

    private:
        std::string m_function_name;
        std::string m_file_name;
        int m_line_number;
    };
}
#endif

#ifdef TRACING_ENABLED
#define TRACE() tracing::Tracer _tracer_object__ ## __COUNTER__ {__func__, __FILE__, __LINE__}
#else
#define TRACE()
#endif
