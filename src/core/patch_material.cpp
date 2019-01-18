
#include "patch_material.hpp"
#include "d3d11_helpers.hpp"

namespace sp::core {

namespace {

auto init_textures(const std::vector<std::string>& textures,
                   const Texture_database& texture_database) noexcept
   -> std::vector<Com_ptr<ID3D11ShaderResourceView>>
{
   std::vector<Com_ptr<ID3D11ShaderResourceView>> resources;

   for (const auto& texture : textures) {
      if (texture.empty()) {
         resources.emplace_back(nullptr);
         continue;
      }

      resources.emplace_back(texture_database.get(texture));
   }

   return resources;
}

auto init_fail_safe_texture(const std::vector<Com_ptr<ID3D11ShaderResourceView>>& shader_resources,
                            const std::int32_t fail_safe_texture_index) noexcept -> Game_texture
{
   Expects(fail_safe_texture_index == -1 ||
           fail_safe_texture_index < shader_resources.size());

   if (fail_safe_texture_index == -1) return {};

   return {shader_resources[fail_safe_texture_index],
           shader_resources[fail_safe_texture_index]};
}

}

Patch_material::Patch_material(Material_info material_info,
                               Material_shader_factory& shader_factory,
                               const Texture_database& texture_database,
                               ID3D11Device1& device) noexcept
   : overridden_rendertype{material_info.overridden_rendertype},
     shader{shader_factory.create(material_info.rendertype)},
     constant_buffer{
        create_immutable_constant_buffer(device, material_info.constant_buffer)},
     shader_resources{init_textures(material_info.textures, texture_database)},
     fail_safe_game_texture{
        init_fail_safe_texture(shader_resources, material_info.fail_safe_texture_index)},
     name{std::move(material_info.name)}
{
}

}
