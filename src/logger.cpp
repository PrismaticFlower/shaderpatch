
#include "logger.hpp"
#include "shader_patch_version.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>

namespace sp::logger_detail {

namespace {

using namespace std::literals;

const auto logger_path = "shader patch.log"s;

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

struct Logger {
   std::mutex mutex;
   std::ofstream file;

   Logger() noexcept
   {
      file.open(logger_path);
      file << "Shader Patch log started. Shader Patch version is "sv
           << current_shader_patch_version_string << std::endl;
   }

   Logger(const Logger&) = delete;
   Logger& operator=(const Logger&) = delete;

   Logger(Logger&&) = delete;
   Logger& operator=(Logger&&) = delete;
};

auto get_logger() noexcept -> Logger&
{
   static Logger logger;

   return logger;
}

}

void log_write(const Log_level level, const bool flush, const std::string_view str) noexcept
{
   auto& logger = get_logger();

   std::scoped_lock lock{logger.mutex};

   const auto time = std::time(nullptr);
   const auto local_time = std::localtime(&time);

   logger.file << level << ' ' << std::put_time(local_time, "%T") << ' ';
   logger.file << str;
   logger.file << '\n';

   if (flush) logger.file.flush();
}

}
