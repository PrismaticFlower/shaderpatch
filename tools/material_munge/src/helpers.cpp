
#include "helpers.hpp"
#include "req_file_helpers.hpp"
#include "synced_io.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>

#include <gsl/gsl>

#include <stb_image.h>

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

auto find_texture_references(const fs::path& from)
   -> std::unordered_map<Ci_string, std::vector<fs::path>>
{
   Expects(fs::is_directory(from));

   std::unordered_map<Ci_string, std::vector<fs::path>> results;

   for (auto& entry : fs::directory_iterator{from}) {
      auto& path = entry.path();

      if (path.extension() != ".req"s) continue;

      try {
         for (auto& section : parse_req_file(path)) {
            if (section.first != "texture"sv) continue;

            for (auto& texture : section.second) {
               results[make_ci_string(texture)].emplace_back(path.parent_path() /
                                                             path.stem());
            }
         }
      }
      catch (std::runtime_error& e) {
         synced_print("Error occured while parsing "sv, path, "\n   mesage: "sv,
                      e.what(),
                      "\n\n   You may need to Clean or Manual Clean your munge files."sv);
      }
   }

   return results;
}

auto load_material_descriptions(const std::vector<std::string>& directories)
   -> std::unordered_map<Ci_string, YAML::Node>
{
   std::unordered_map<Ci_string, YAML::Node> results;

   for (fs::path path : directories) {
      if (!fs::exists(path) || !fs::is_directory(path)) {
         synced_error_print("Warning specified material description directory "sv,
                            path, " does not exist!"sv);
      }

      for (auto entry : fs::directory_iterator{path}) {
         try {
            results[make_ci_string(entry.path().stem().string())] =
               YAML::LoadFile(entry.path().string());
         }
         catch (std::exception& e) {
            synced_error_print("Error failed to read material description "sv,
                               entry.path(), " message: "sv, e.what());
         }
      }
   }

   return results;
}
}
