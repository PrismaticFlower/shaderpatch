#pragma once

#include "shader_patch_version.hpp"

#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>

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

   friend inline Logger& get_logger() noexcept;

   void write(const std::string_view what) noexcept
   {
      _log_file << what << '\n';
   }

   void flush() noexcept
   {
      _log_file.flush();
   }

private:
   Logger() noexcept
   {
      _log_file.open(logger_path);

      _log_file << "Shader Patch log started. Shader Patch version is "sv
                << current_shader_patch_version_string << '\n';
   }

   std::ofstream _log_file;
};

inline Logger& get_logger() noexcept
{
   static Logger logger;

   return logger;
}

template<typename... Args>
inline void log(const Log_level level, Args&&... args) noexcept
{
   auto& logger = get_logger();

   std::stringstream stream;

   const auto time = std::time(nullptr);
   const auto local_time = std::localtime(&time);

   stream << level << ' ' << std::put_time(local_time, "%T") << ' ';

   // TODO: Switch this to a fold expression once VC++ supports them.
   using std::operator<<;

   const auto write_out = [&](auto&& arg) {
      stream << std::forward<decltype(arg)>(arg);
   };

   [[maybe_unused]] const bool dummy_list[] = {
      (write_out(std::forward<Args>(args)), false)...};

   // stream << ... << args;

   logger.write(stream.str());

   if (level == Log_level::error) logger.flush();
}

template<typename... Args>
[[noreturn]] inline void log_and_terminate(Args&&... args)
{
   log(Log_level::error, std::forward<Args>(args)...);

   std::terminate();
}
}

#pragma warning(pop)
