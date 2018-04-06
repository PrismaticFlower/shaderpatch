#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>

namespace sp {

namespace fs = boost::filesystem;

auto find_texture_references(const fs::path& from)
   -> std::unordered_map<std::string, std::vector<fs::path>>;

auto build_input_file_map(const fs::path& in)
   -> std::unordered_map<std::string, fs::path>;
}
