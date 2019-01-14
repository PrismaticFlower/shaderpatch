
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
}

Patch_material::Patch_material(Material_info material_info,
                               const Shader_rendertype_collection& rendertypes,
                               const Texture_database& texture_database,
                               ID3D11Device1& device) noexcept
   : overridden_rendertype{material_info.overridden_rendertype},
     shaders{rendertypes.at(material_info.rendertype)},
     constant_buffer{
        create_immutable_constant_buffer(device, material_info.constant_buffer)},
     shader_resources{init_textures(material_info.textures, texture_database)},
     name{std::move(material_info.name)}
{
}

}
