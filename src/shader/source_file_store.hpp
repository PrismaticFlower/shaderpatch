#pragma once

#include "../logger.hpp"
#include "file_helpers.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <absl/container/flat_hash_map.h>

namespace sp::shader {

class Source_file_store {
public:
   explicit Source_file_store(const std::filesystem::path& source_path) noexcept
   {
      for (const auto& entry : std::filesystem::directory_iterator{source_path}) {
         if (!entry.is_regular_file()) continue;

         _files[entry.path().filename().string()] =
            File{.data = load_string_file(entry), .path = entry.path().string()};
      }
   }

   auto data(const std::string_view name) const noexcept
      -> std::optional<std::string_view>
   {
      if (auto it = _files.find(name); it != _files.end()) {
         return it->second.data;
      }

      log(Log_level::error, "Failed to find file '"sv, name,
          "' in shader source file store. This may be caused by the case sensitivty of the source file store."sv);

      return std::nullopt;
   }

   auto file_path(const std::string_view name) const noexcept -> const std::string&
   {
      if (auto it = _files.find(name); it != _files.end()) {
         return it->second.path;
      }

      log_and_terminate("Failed to find file '"sv, name,
                        "' in shader source file store. This may be caused by the case sensitivty of the source file store."sv);
   }

private:
   struct File {
      std::string data;
      std::string path;
   };

   absl::flat_hash_map<std::string, File> _files;
};

}
