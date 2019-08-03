#pragma once

#include "terrain_map.hpp"

namespace sp {

void terrain_save_normal_map(const Terrain_map& terrain, const std::string_view suffix,
                             const std::filesystem::path& output_munge_files_dir);

}
