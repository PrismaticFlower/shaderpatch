#pragma once

#include "munge_helpers.hpp"

#include <gsl/gsl>

namespace sp {

auto build_input_file_map(const std::filesystem::path& in)
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
