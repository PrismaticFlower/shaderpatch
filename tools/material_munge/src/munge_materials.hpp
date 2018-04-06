#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>

namespace sp {

namespace fs = boost::filesystem;

void munge_materials(const fs::path& output_path,
                     const std::unordered_map<std::string, std::vector<fs::path>>& texture_references,
                     const std::unordered_map<std::string, fs::path>& files);
}
