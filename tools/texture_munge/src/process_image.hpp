#pragma once

#include "patch_texture.hpp"

#include <cstddef>
#include <filesystem>
#include <tuple>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto process_image(YAML::Node config, std::filesystem::path image_file_path)
   -> std::tuple<Texture_info, std::vector<std::vector<std::byte>>>;
}
