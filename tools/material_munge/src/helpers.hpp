#pragma once

#include "string_utilities.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

namespace fs = boost::filesystem;

auto find_texture_references(const fs::path& from)
   -> std::unordered_map<Ci_string, std::vector<fs::path>>;

auto build_input_file_map(const fs::path& in)
   -> std::unordered_map<Ci_string, fs::path>;

auto load_material_descriptions(const std::vector<std::string>& directories)
   -> std::unordered_map<Ci_string, YAML::Node>;
}
