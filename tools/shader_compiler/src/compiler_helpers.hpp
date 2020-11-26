#pragma once

#include <filesystem>
#include <fstream>

#include <gsl/gsl>
#include <nlohmann/json.hpp>

namespace sp {

inline auto read_json_file(const std::filesystem::path& path) -> nlohmann::json
{
   namespace fs = std::filesystem;

   std::ifstream file{path.string()};

   nlohmann::json config;
   file >> config;

   return config;
}

}
