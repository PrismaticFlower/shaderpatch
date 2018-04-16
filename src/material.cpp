
#include "material.hpp"
#include "logger.hpp"
#include "shader_constants.hpp"

#include <string_view>

#include <glm/gtc/type_ptr.hpp>

namespace sp {

namespace {

auto resolve_textures(const Material_info& info, const Texture_database& textures)
{
   std::array<Texture, Material::max_material_textures> resolved;

   for (auto i = 0; i < Material::max_material_textures; ++i) {
      if (info.textures[i].empty()) continue;

      resolved[i] = textures.get(info.textures[i]);
   }

   return resolved;
}
}

Material::Material(const Material_info& info,
                   gsl::not_null<Com_ptr<IDirect3DDevice9>> device,
                   const Shader_database& shaders, const Texture_database& textures) noexcept
   : _device{std::move(device)},
     _overridden_rendertype{info.overridden_rendertype},
     _constants{info.constants},
     _textures{resolve_textures(info, textures)},
     _shader_group{shaders.at(info.rendertype)}
{
}

auto Material::target_rendertype() const noexcept -> std::string_view
{
   return _overridden_rendertype;
}

void Material::use(const std::string& entrypoint, const Shader_flags flags) const noexcept
{
   for (auto i = 0; i < material_textures_offset; ++i) {
      _textures[i].bind(material_textures_offset + i);
   }

   _device->SetVertexShaderConstantF(sp::constants::vs::material_constants_start,
                                     glm::value_ptr(_constants[0]),
                                     max_material_constants);

   _device->SetPixelShaderConstantF(sp::constants::ps::material_constants_start,
                                    glm::value_ptr(_constants[0]),
                                    max_material_constants);
   update(entrypoint, flags);
}

void Material::update(const std::string& entrypoint, const Shader_flags flags) const noexcept
{
   const auto& program = _shader_group.at(entrypoint)[flags];

   program.bind(*_device);
}
}
