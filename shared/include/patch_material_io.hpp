#pragma once

#include "game_rendertypes.hpp"
#include "ucfb_reader.hpp"
#include "utility.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace sp {

struct Material_info {
   std::string name;
   std::string rendertype;
   Rendertype overridden_rendertype;
   Aligned_vector<std::byte, 16> constant_buffer{};
   std::vector<std::string> textures{};
   std::int32_t fail_safe_texture_index;
};

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_info& info);

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_info;

}
