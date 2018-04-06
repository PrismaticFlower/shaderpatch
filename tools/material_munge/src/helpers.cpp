
#include "helpers.hpp"
#include "req_file_helpers.hpp"
#include "synced_io.hpp"

#include <iostream>
#include <string_view>

#include <gsl/gsl>

#include <stb_image.h>

namespace sp {

using namespace std::literals;
namespace fs = boost::filesystem;

auto find_texture_references(const fs::path& from)
   -> std::unordered_map<std::string, std::vector<std::string>>
{
   Expects(fs::is_directory(from));

   std::unordered_map<std::string, std::vector<std::string>> results;

   for (auto& entry : fs::directory_iterator{from}) {
      auto& path = entry.path();

      if (path.extension() != ".req"s) continue;

      try {
         const auto file_name = path.stem().string();

         for (auto& section : parse_req_file(path)) {
            if (section.first != "texture"sv) continue;

            for (auto& texture : section.second) {
               results[texture].emplace_back(file_name);
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

auto build_input_file_map(const fs::path& in)
   -> std::unordered_map<std::string, fs::path>
{
   Expects(fs::is_directory(in));

   std::unordered_map<std::string, fs::path> results;

   for (auto& entry : fs::recursive_directory_iterator{in}) {
      if (!fs::is_regular_file(entry.path())) continue;

      results[entry.path().filename().string()] = entry.path();
   }

   return results;
}
}
