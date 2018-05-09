#pragma once

#include "string_utilities.hpp"

#include <filesystem>
#include <unordered_map>

#include <gsl/gsl>

namespace sp {

inline auto build_input_file_map(const std::filesystem::path& in)
   -> std::unordered_map<Ci_string, std::filesystem::path>
{
   namespace fs = std::filesystem;

   Expects(fs::is_directory(in));

   std::unordered_map<Ci_string, fs::path> results;

   for (auto& entry : fs::recursive_directory_iterator{in}) {
      if (!fs::is_regular_file(entry.path())) continue;

      results[make_ci_string(entry.path().filename().string())] = entry.path();
   }

   return results;
}
}
