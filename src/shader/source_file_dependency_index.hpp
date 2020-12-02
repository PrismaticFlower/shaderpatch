#pragma once

#include "source_file_store.hpp"
#include "string_utilities.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <absl/container/flat_hash_map.h>

namespace sp::shader {

class Source_file_dependency_index {
public:
   Source_file_dependency_index() = default;

   explicit Source_file_dependency_index(const Source_file_store& source_files)
   {
      rescan(source_files);
   }

   void rescan(const Source_file_store& source_files) noexcept
   {
      for (const auto& file : source_files.get_range()) {
         rescan_entry(file.name, file.data);
      }
   }

   auto operator[](const std::string_view source_file) const noexcept
      -> std::span<const std::string>
   {
      auto files = _dependencies_map.find(source_file);

      return files != _dependencies_map.cend() ? files->second
                                               : std::span<const std::string>{};
   }

private:
   void rescan_entry(const std::string_view name, const std::string_view data) noexcept
   {
      using namespace std::literals;

      auto& dependencies = _dependencies_map[name];

      dependencies.clear();

      for (auto line : Lines_iterator{data}) {
         if (!line.string.starts_with("#include"sv)) continue;

         auto included_file =
            split_string_on(split_string_on(line.string, "\""sv)[1], "\""sv)[0];

         dependencies.emplace_back(included_file);
      }
   }

   absl::flat_hash_map<std::string, std::vector<std::string>> _dependencies_map;
};

}
