#pragma once

#include "enum_flags.hpp"
#include "game_rendertypes.hpp"
#include "ucfb_reader.hpp"
#include "utility.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <d3d11_1.h>

namespace sp {

enum class Material_cb_shader_stages : std::uint32_t {
   none = 0b0u,
   vs = 0b10u,
   hs = 0b100u,
   ds = 0b1000u,
   gs = 0b10000u,
   ps = 0b100000u
};

constexpr bool marked_as_enum_flag(Material_cb_shader_stages) noexcept
{
   return true;
}

struct Material_info {
   std::string name;
   std::string rendertype;
   Rendertype overridden_rendertype;
   Material_cb_shader_stages cb_shader_stages = Material_cb_shader_stages::none;
   Aligned_vector<std::byte, 16> constant_buffer{};
   std::vector<std::string> vs_textures{};
   std::vector<std::string> hs_textures{};
   std::vector<std::string> ds_textures{};
   std::vector<std::string> gs_textures{};
   std::vector<std::string> ps_textures{};
   std::uint32_t fail_safe_texture_index{};
   bool tessellation = false;
   D3D11_PRIMITIVE_TOPOLOGY tessellation_primitive_topology =
      D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
};

void write_patch_material(const std::filesystem::path& save_path,
                          const Material_info& info);

auto read_patch_material(ucfb::Reader_strict<"matl"_mn> reader) -> Material_info;

}
