#pragma once

#include "patch_texture.hpp"

#include <cstddef>
#include <tuple>
#include <vector>

#include <boost/filesystem.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto process_image(YAML::Node config, boost::filesystem::path image_file_path)
   -> std::tuple<Texture_info, std::vector<std::vector<std::byte>>>;
}
