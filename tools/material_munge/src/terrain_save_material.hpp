#pragma once

#include <array>
#include <filesystem>
#include <string_view>

#include "terrain_materials_config.hpp"
#include "terrain_texture_transform.hpp"

namespace sp {

void terrain_save_material(const Terrain_materials_config& config,
                           const std::array<Terrain_texture_transform, 16>& texture_transforms,
                           const std::vector<std::string> textures_order,
                           const std::string_view suffix,
                           const std::filesystem::path& output_munge_files_dir);
}
