#pragma once

#include "string_utilities.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto find_texture_references(const std::filesystem::path& from)
   -> std::unordered_map<Ci_string, std::vector<std::filesystem::path>>;

auto load_material_descriptions(const std::vector<std::string>& directories)
   -> std::unordered_map<Ci_string, YAML::Node>;
}
