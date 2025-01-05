#include "logger.hpp"
#include "shader_patch_version.hpp"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <shared_mutex>

namespace sp {

namespace {

auto operator<<(std::ostream& stream, const Log_level& level) noexcept -> std::ostream&
{
   switch (level) {
   case Log_level::debug:
      return stream << "[DEBUG]  "sv;
   case Log_level::info:
      return stream << "[INFO]   "sv;
   case Log_level::warning:
      return stream << "[WARNING]"sv;
   case Log_level::error:
      return stream << "[ERROR]  "sv;
   }

   return stream;
}

class Logger_impl : public Logger {
public:
   Logger_impl() noexcept
   {
      _log_file.open(logger_path);
      _log_file << "Shader Patch log started. Shader Patch version is "sv
                << current_shader_patch_version_string << std::endl;
   }

   void write_ln(Log_level level, std::string_view str) noexcept override
   {
      std::scoped_lock lock{_log_mutex};

      const auto time = std::time(nullptr);
      const auto local_time = std::localtime(&time);

      _log_file << level << ' ' << std::put_time(local_time, "%T") << ' ' << str
                << '\n';

      if (level != Log_level::debug) _log_file.flush();
   }

private:
   std::shared_mutex _log_mutex;
   std::ofstream _log_file;
};
}

auto get_logger() noexcept -> Logger&
{
   static Logger_impl logger;

   return logger;
}

}