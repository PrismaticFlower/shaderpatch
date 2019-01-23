#pragma once

#include "string_utilities.hpp"

#include <filesystem>
#include <unordered_map>

namespace sp {

auto build_input_file_map(const std::filesystem::path& in)
   -> std::unordered_map<Ci_string, std::filesystem::path>;
}
