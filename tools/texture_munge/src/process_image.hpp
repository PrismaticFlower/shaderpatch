#pragma once

#include <cstddef>
#include <filesystem>
#include <tuple>
#include <vector>

#include <DirectXTex.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto process_image(const YAML::Node& config, std::filesystem::path image_file_path)
   -> DirectX::ScratchImage;
}
