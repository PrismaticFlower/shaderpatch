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

void munge_materials(
   const std::filesystem::path& output_path,
   const std::unordered_map<Ci_string, std::vector<std::filesystem::path>>& texture_references,
   const std::unordered_map<Ci_string, std::filesystem::path>& files,
   const std::unordered_map<Ci_string, YAML::Node>& descriptions,
   const bool patch_material_flags);
}
