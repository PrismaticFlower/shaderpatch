#pragma once

#include <exception>
#include <iomanip>
#include <sstream>
#include <string_view>

#include <fmt/format.h>

namespace sp {

using namespace std::literals;

const auto logger_path = "shader patch.log"s;

enum class Log_level { debug, info, warning, error };

class Logger {
public:
   virtual void write_ln(Log_level level, std::string_view str) noexcept = 0;
};

auto get_logger() noexcept -> Logger&;

template<typename... Args>
inline void log(const Log_level level, const Args&... args) noexcept
{
   std::ostringstream stream;

   (stream << ... << args);

   get_logger().write_ln(level, stream.view());
}

template<typename... Args>
inline void log_debug([[maybe_unused]] fmt::format_string<const Args&...> format_str,
                      [[maybe_unused]] const Args&... args) noexcept
{
#ifndef NDEBUG
   get_logger().write_ln(Log_level::debug,
                         fmt::vformat(format_str, fmt::make_format_args(args...)));
#endif
}

template<typename... Args>
inline void log_fmt(const Log_level level, fmt::format_string<const Args&...> format_str,
                    const Args&... args) noexcept
{
   get_logger().write_ln(level,
                         fmt::vformat(format_str, fmt::make_format_args(args...)));
}

template<typename... Args>
[[noreturn]] inline void log_and_terminate(const Args&... args)
{
   log(Log_level::error, args...);

   std::terminate();
}

template<typename... Args>
[[noreturn]] inline void log_and_terminate_fmt(fmt::format_string<const Args&...> format_str,
                                               const Args&... args)
{
   log_fmt(Log_level::error, format_str, args...);

   std::terminate();
}

}
