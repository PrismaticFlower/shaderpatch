#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "terrain_materials_config.hpp"

namespace sp {

void terrain_assemble_textures(const Terrain_materials_config& config,
                               const std::string_view texture_suffix,
                               const std::vector<std::string>& materials,
                               const std::filesystem::path& output_dir,
                               const std::filesystem::path& sptex_files_dir);

}
