
#include "terrain_save_material.hpp"
#include "patch_material_io.hpp"
#include "req_file_helpers.hpp"
#include "terrain_constants.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

using namespace std::literals;

namespace sp {

struct Terrain_material_constants {
   glm::vec3 base_color;
   float base_metallicness;
   float base_roughness;
   glm::vec3 _padding{};

   struct Texture_transform {
      glm::vec3 row0;
      float _padding0{};
      glm::vec3 row1;
      float _padding1{};
   };

   std::array<Texture_transform, 16> texture_transforms;

   std::array<glm::vec4, 16> texture_height_scales;
};

static_assert(sizeof(Terrain_material_constants) == 800);

namespace {

auto create_constant_buffer(const Terrain_materials_config& config,
                            const std::array<Terrain_texture_transform, 16>& texture_transforms,
                            const std::vector<std::string> textures_order)
   -> Aligned_vector<std::byte, 16>
{
   Aligned_vector<std::byte, 16> cb_data;
   cb_data.resize(sizeof(Terrain_material_constants));

   auto* const cb = new (cb_data.data()) Terrain_material_constants{};

   cb->base_color = config.base_color;
   cb->base_metallicness = config.base_metallicness;
   cb->base_roughness = config.base_roughness;

   for (auto i = 0; i < texture_transforms.size(); ++i) {
      const auto transform = texture_transforms[i].get_transform();
      cb->texture_transforms[i].row0 = transform[0];
      cb->texture_transforms[i].row1 = transform[1];
   }

   for (auto i = 0; i < safe_min(textures_order.size(), std::size_t{16}); ++i) {
      auto it = config.materials.find(textures_order[i]);

      if (it == config.materials.cend()) continue;

      cb->texture_height_scales[i].x = it->second.height_scale;
   }

   return cb_data;
}

auto select_rendertype(const Terrain_materials_config& config) noexcept -> std::string
{
   std::string rt = config.rendertype == Terrain_rendertype::pbr
                       ? "pbr_terrain"s
                       : "normal_ext_terrain"s;

   if (config.bumpmapping == Terrain_bumpmapping::parallax_offset_mapping) {
      rt += ".parallax offset mapping"s;
   }
   else if (config.bumpmapping == Terrain_bumpmapping::parallax_occlusion_mapping) {
      rt += ".parallax occlusion mapping"s;
   }

   return rt;
}

auto select_rendertype_low_detail(const Terrain_materials_config& config) noexcept
   -> std::string
{
   std::string rt = config.rendertype == Terrain_rendertype::pbr
                       ? "pbr_terrain.low detail"s
                       : "normal_ext_terrain.low detail"s;

   return rt;
}

auto select_textures(const Terrain_materials_config& config,
                     const std::string_view suffix) -> std::vector<std::string>
{
   if (config.rendertype == Terrain_rendertype::pbr) {
      return {std::string{terrain_normal_map_texture_name} += suffix,
              std::string{terrain_height_texture_name} += suffix,
              std::string{terrain_albedo_ao_texture_name} += suffix,
              std::string{terrain_normal_mr_texture_name} += suffix};
   }
   else {
      return {std::string{terrain_normal_map_texture_name} += suffix,
              std::string{terrain_height_texture_name} += suffix,
              std::string{terrain_diffuse_ao_texture_name} += suffix,
              std::string{terrain_normal_gloss_texture_name} += suffix};
   }
}

}

void terrain_save_material(const Terrain_materials_config& config,
                           const std::array<Terrain_texture_transform, 16>& texture_transforms,
                           const std::vector<std::string> textures_order,
                           const std::string_view suffix,
                           const std::filesystem::path& output_munge_files_dir)
{
   Material_info info;

   info.name = terrain_material_name;
   info.name += suffix;
   info.rendertype = select_rendertype(config);
   info.overridden_rendertype = Rendertype::normal;
   info.cb_shader_stages =
      Material_cb_shader_stages::vs | Material_cb_shader_stages::ps;
   info.constant_buffer =
      create_constant_buffer(config, texture_transforms, textures_order);
   info.ps_textures = select_textures(config, suffix);
   info.fail_safe_texture_index = 1;

   write_patch_material(output_munge_files_dir / info.name += ".material"sv, info);
   emit_req_file(output_munge_files_dir / info.name += ".material.req"sv,
                 {{"sptex"s, info.ps_textures}});

   // Save low detail material

   info.name += terrain_low_detail_suffix;
   info.rendertype = select_rendertype_low_detail(config);

   write_patch_material(output_munge_files_dir / info.name += ".material"sv, info);
   emit_req_file(output_munge_files_dir / info.name += ".material.req"sv,
                 {{"sptex"s, info.ps_textures}});
}
}
