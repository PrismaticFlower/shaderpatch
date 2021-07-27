#pragma once

#include "../logger.hpp"
#include "file_helpers.hpp"

#include <filesystem>
#include <optional>
#ifndef __INTELLISENSE__
#include <ranges>
#endif
#include <string>
#include <string_view>

#include <absl/container/flat_hash_map.h>

namespace sp::shader {

class Source_file_store {
public:
   struct File_ref {
      std::string_view name;
      std::string_view data;
      std::filesystem::file_time_type last_write_time;
   };

   explicit Source_file_store(const std::filesystem::path& source_path) noexcept
   {
      for (const auto& entry : std::filesystem::directory_iterator{source_path}) {
         if (!entry.is_regular_file()) continue;

         _files[entry.path().filename().string()] =
            File{.data = load_string_file(entry),
                 .path = entry.path().string(),
                 .last_write_time = entry.last_write_time()};
      }
   }

   auto data(const std::string_view name) const noexcept
      -> std::optional<std::string_view>
   {
      if (auto it = _files.find(name); it != _files.end()) {
         return it->second.data;
      }

      log(Log_level::error,
          "Failed to find file '{}' in shader source file store. This may be caused by the case sensitivty of the source file store."sv,
          name);

      return std::nullopt;
   }

   auto file_path(const std::string_view name) const noexcept -> const std::string&
   {
      if (auto it = _files.find(name); it != _files.end()) {
         return it->second.path;
      }

      log_and_terminate("Failed to find file '{}' in shader source file store. This may be caused by the case sensitivty of the source file store."sv,
                        name);
   }

   auto last_write_time(const std::string_view name) const noexcept
      -> std::filesystem::file_time_type
   {
      if (auto it = _files.find(name); it != _files.end()) {
         return it->second.last_write_time;
      }

      log_and_terminate("Failed to find file '{}' in shader source file store. This may be caused by the case sensitivty of the source file store."sv,
                        name);
   }

#ifndef __INTELLISENSE__
   auto get_range() const noexcept
   {
      return _files | std::views::transform([](const auto& file) noexcept {
                return File_ref{.name = file.first,
                                .data = file.second.data,
                                .last_write_time = file.second.last_write_time};
             });
   }
#endif

   auto size() const noexcept
   {
      return _files.size();
   }

   void reload() noexcept
   {
      for (auto& [name, file] : _files) file.data = load_string_file(file.path);
   }

private:
   struct File {
      std::string data;
      std::string path;
      std::filesystem::file_time_type last_write_time;
   };

   absl::flat_hash_map<std::string, File> _files;
};

}
