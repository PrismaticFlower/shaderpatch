#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <unordered_map>

namespace sp {

struct Atlas_location {
   float left;
   float right;
   float top;
   float bottom;
};

auto read_atlas_description(const std::filesystem::path& path)
   -> std::unordered_map<char16_t, Atlas_location>;

}