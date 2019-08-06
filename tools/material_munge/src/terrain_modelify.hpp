#pragma once

#include "terrain_map.hpp"

#include <filesystem>

namespace sp {

void terrain_modelify(const Terrain_map& terrain, const std::string_view material_suffix,
                      const bool keep_static_lighting,
                      const std::filesystem::path& munged_input_terrain_path,
                      const std::filesystem::path& output_path);

}
