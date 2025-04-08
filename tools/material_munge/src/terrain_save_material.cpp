
#include "terrain_save_material.hpp"
#include "patch_material_io.hpp"
#include "req_file_helpers.hpp"
#include "string_utilities.hpp"
#include "terrain_constants.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>

using namespace std::literals;

namespace sp {

namespace {

enum class Displacement_mode : std::int32_t {
   none = 0,
   parallax_offset = 1,
   occlusion_mapping = 2
};

enum class Blend_modes : std::int32_t { height = 0, basic = 1 };

auto add_texture_transform_properties(std::vector<Material_property>& properties,
                                      const std::array<Terrain_texture_transform, 16>& texture_transforms)
{
   for (auto i = 0; i < texture_transforms.size(); ++i) {
      const auto transform = texture_transforms[i].get_transform();

      properties.emplace_back("TextureTransformsX"s + std::to_string(i), transform[0],
                              glm::vec3{std::numeric_limits<float>::lowest()},
                              glm::vec3{std::numeric_limits<float>::max()});
      properties.emplace_back("TextureTransformsY"s + std::to_string(i), transform[1],
                              glm::vec3{std::numeric_limits<float>::lowest()},
                              glm::vec3{std::numeric_limits<float>::max()});
   }
}

void add_common_properties(std::vector<Material_property>& properties,
                           const Terrain_materials_config& config, const bool low_detail)
{
   if (!low_detail && config.bumpmapping == Terrain_bumpmapping::parallax_offset_mapping) {
      properties.emplace_back("DisplacementMode"s,
                              static_cast<std::int32_t>(Displacement_mode::parallax_offset));
   }
   else if (!low_detail && config.bumpmapping ==
                              Terrain_bumpmapping::parallax_occlusion_mapping) {
      properties.emplace_back("DisplacementMode"s,
                              static_cast<std::int32_t>(
                                 Displacement_mode::occlusion_mapping));
   }
   else {
      properties.emplace_back("DisplacementMode"s,
                              static_cast<std::int32_t>(Displacement_mode::none));
   }

   if (!low_detail && config.blending == Terrain_blending::basic) {
      properties.emplace_back("BlendMode"s,
                              static_cast<std::int32_t>(Blend_modes::basic));
   }
   else {
      properties.emplace_back("BlendMode"s,
                              static_cast<std::int32_t>(Blend_modes::height));
   }

   if (low_detail) properties.emplace_back("LowDetail"s, true);
}

auto create_pbr_properties(const Terrain_materials_config& config,
                           const std::array<Terrain_texture_transform, 16>& texture_transforms,
                           const std::vector<std::string>& textures_order,
                           const bool low_detail) -> std::vector<Material_property>
{
   std::vector<Material_property> properties;
   properties.reserve(96);

   properties.emplace_back("BaseColor"s, config.base_color, glm::vec3{0.0f},
                           glm::vec3{1.0f});
   properties.emplace_back("BaseMetallicness"s, config.base_metallicness, 0.0f, 1.0f);
   properties.emplace_back("BaseRoughness"s, config.base_roughness, 0.0f, 1.0f);

   add_common_properties(properties, config, low_detail);
   add_texture_transform_properties(properties, texture_transforms);

   for (auto i = 0; i < safe_min(textures_order.size(), std::size_t{16}); ++i) {
      auto it =
         std::ranges::find_if(config.materials,
                              [texture_name = view_as_ci_string(textures_order[i])](
                                 const std::pair<std::string, Terrain_material>& name_material) {
                                 return name_material.first == texture_name;
                              });

      if (it == config.materials.cend()) continue;

      properties.emplace_back("HeightScale"s + std::to_string(i),
                              it->second.height_scale, 0.0f, 2048.0f);
   }

   return properties;
}

auto create_normal_ext_properties(const Terrain_materials_config& config,
                                  const std::array<Terrain_texture_transform, 16>& texture_transforms,
                                  const std::vector<std::string>& textures_order,
                                  const bool low_detail) -> std::vector<Material_property>
{
   std::vector<Material_property> properties;
   properties.reserve(96);

   properties.emplace_back("DiffuseColor"s, config.diffuse_color,
                           glm::vec3{0.0f}, glm::vec3{1.0f});
   properties.emplace_back("SpecularColor"s, config.specular_color,
                           glm::vec3{0.0f}, glm::vec3{1.0f});
   properties.emplace_back("UseEnvmap"s, config.use_envmap, false, true);

   add_common_properties(properties, config, low_detail);
   add_texture_transform_properties(properties, texture_transforms);

   for (auto i = 0; i < safe_min(textures_order.size(), std::size_t{16}); ++i) {
      auto it =
         std::ranges::find_if(config.materials,
                              [texture_name = view_as_ci_string(textures_order[i])](
                                 const std::pair<std::string, Terrain_material>& name_material) {
                                 return name_material.first == texture_name;
                              });

      if (it == config.materials.cend()) continue;

      properties.emplace_back("HeightScale"s + std::to_string(i),
                              it->second.height_scale, 0.0f, 2048.0f);

      properties.emplace_back("SpecularExponent"s + std::to_string(i),
                              it->second.specular_exponent, 1.0f, 2048.0f);
   }

   return properties;
}

auto create_properties(const Terrain_materials_config& config,
                       const std::array<Terrain_texture_transform, 16>& texture_transforms,
                       const std::vector<std::string>& textures_order,
                       const bool low_detail) -> std::vector<Material_property>
{
   if (config.rendertype == Terrain_rendertype::pbr) {
      return create_pbr_properties(config, texture_transforms, textures_order,
                                   low_detail);
   }
   else {
      return create_normal_ext_properties(config, texture_transforms,
                                          textures_order, low_detail);
   }
}

auto select_material_type(const Terrain_materials_config& config) noexcept -> std::string
{
   return config.rendertype == Terrain_rendertype::pbr ? "pbr_terrain"s
                                                       : "normal_ext_terrain"s;
}

auto select_textures(const Terrain_materials_config& config, const std::string_view suffix)
   -> absl::flat_hash_map<std::string, std::string>
{
   if (config.rendertype == Terrain_rendertype::pbr) {
      return {{"height_textures"s, std::string{terrain_height_texture_name} += suffix},
              {"albedo_ao_textures"s,
               std::string{terrain_albedo_ao_texture_name} += suffix},
              {"normal_mr_textures"s,
               std::string{terrain_normal_mr_texture_name} += suffix}};
   }
   else {
      return {{"height_textures"s, std::string{terrain_height_texture_name} += suffix},
              {"diffuse_ao_textures"s,
               std::string{terrain_diffuse_ao_texture_name} += suffix},
              {"normal_gloss_textures"s,
               std::string{terrain_normal_gloss_texture_name} += suffix},
              {"envmap"s, config.use_envmap ? config.envmap_name : ""s}};
   }
}

auto get_req_contents(const absl::flat_hash_map<std::string, std::string>& resources)
   -> std::vector<std::string>
{
   std::vector<std::string> vec;
   vec.reserve(resources.size());

   for (const auto& [key, value] : resources) vec.push_back(value);

   return vec;
}

}

void terrain_save_material(const Terrain_materials_config& config,
                           const std::array<Terrain_texture_transform, 16>& texture_transforms,
                           const std::vector<std::string> textures_order,
                           const std::string_view suffix,
                           const std::filesystem::path& output_munge_files_dir)
{
   Material_config mtrl;

   mtrl.name = terrain_material_name;
   mtrl.name += suffix;
   mtrl.type = select_material_type(config);
   mtrl.overridden_rendertype = Rendertype::normal;
   mtrl.properties =
      create_properties(config, texture_transforms, textures_order, false);
   mtrl.resources = select_textures(config, suffix);

   const auto req_contents = get_req_contents(mtrl.resources);

   write_patch_material(output_munge_files_dir / mtrl.name += ".material"sv, mtrl);
   emit_req_file(output_munge_files_dir / mtrl.name += ".material.req"sv,
                 {{"sptex"s, req_contents}});
}
}
