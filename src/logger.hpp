#pragma once

#include <exception>
#include <string_view>

#include <fmt/format.h>

namespace sp {

enum class Log_level { info, warning, error };

namespace logger_detail {

void log_write(const Log_level level, const bool flush,
               const std::string_view str) noexcept;

}

inline void log(const Log_level level, const std::string_view str) noexcept
{
   logger_detail::log_write(level, true, str);
}

template<typename... Args>
inline void log_debug([[maybe_unused]] std::string_view format_str,
                      [[maybe_unused]] const Args&... args) noexcept
{
#ifndef NDEBUG
   logger_detail::log_write(Log_level::info, false, fmt::format(format_str, args...));
#endif
}

template<typename... Args>
inline void log(const Log_level level, std::string_view format_str,
                const Args&... args) noexcept
{
   logger_detail::log_write(level, true, fmt::format(format_str, args...));
}

template<typename... Args>
[[noreturn]] inline void log_and_terminate(const std::string_view str)
{
   log(Log_level::error, str);

   std::terminate();
}

template<typename... Args>
[[noreturn]] inline void log_and_terminate(std::string_view format_str,
                                           const Args&... args)
{
   log(Log_level::error, format_str, args...);

   std::terminate();
}

}
