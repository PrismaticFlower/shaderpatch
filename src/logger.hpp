#pragma once

#include "shader_patch_version.hpp"

#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#pragma warning(push)
#pragma warning(disable : 4996)

namespace sp {

using namespace std::literals;

const auto logger_path = "shader patch.log"s;

enum class Log_level { info, warning, error };

inline std::ostream& operator<<(std::ostream& stream, const Log_level& level) noexcept
{
   switch (level) {
   case Log_level::info:
      return stream << "[INFO]   "sv;
   case Log_level::warning:
      return stream << "[WARNING]"sv;
   case Log_level::error:
      return stream << "[ERROR]  "sv;
   }

   return stream;
}

class Logger {
public:
   Logger(const Logger&) = delete;
   Logger& operator=(const Logger&) = delete;

   Logger(Logger&&) = delete;
   Logger& operator=(Logger&&) = delete;

   friend auto get_log_stream() noexcept -> std::ostream&;

private:
   Logger() noexcept
   {
      _log_file.open(logger_path);
      _log_file << "Shader Patch log started. Shader Patch version is "sv
                << current_shader_patch_version_string << std::endl;
   }

   std::ofstream _log_file;
};

inline auto get_log_stream() noexcept -> std::ostream&
{
   static Logger logger;

   return logger._log_file;
}

inline void log(const Log_level level, const std::string_view str) noexcept
{
   auto& stream = get_log_stream();

   const auto time = std::time(nullptr);
   const auto local_time = std::localtime(&time);

   stream << level << ' ' << std::put_time(local_time, "%T") << ' ';
   stream << str;
   stream << std::endl;
}

template<typename... Args>
inline void log_debug([[maybe_unused]] std::string_view format_str,
                      [[maybe_unused]] const Args&... args) noexcept
{
#ifndef NDEBUG
   auto& stream = get_log_stream();

   const auto time = std::time(nullptr);
   const auto local_time = std::localtime(&time);

   stream << Log_level::info << ' ' << std::put_time(local_time, "%T") << ' ';
   stream << fmt::format(format_str, args...);
   stream << '\n';
#endif
}

template<typename... Args>
inline void log(const Log_level level, std::string_view format_str,
                const Args&... args) noexcept
{
   auto& stream = get_log_stream();

   const auto time = std::time(nullptr);
   const auto local_time = std::localtime(&time);

   stream << level << ' ' << std::put_time(local_time, "%T") << ' ';
   stream << fmt::format(format_str, args...);
   stream << std::endl;
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

#pragma warning(pop)
