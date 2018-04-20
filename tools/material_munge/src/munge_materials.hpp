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

void munge_materials(const fs::path& output_path,
                     const std::unordered_map<Ci_string, std::vector<fs::path>>& texture_references,
                     const std::unordered_map<Ci_string, fs::path>& files,
                     const std::unordered_map<Ci_string, YAML::Node>& descriptions);
}
