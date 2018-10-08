
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

auto resolve_constants(const Material_info& info,
                       const std::array<Texture, Material::max_material_textures>& textures) noexcept
{
   auto constants = info.constants;

   for (auto i = 0u; i < info.texture_size_mappings.size(); ++i) {
      const auto& size_offset = info.texture_size_mappings[i];
      const auto& texture = textures[i];

      if (size_offset == -1) continue;

      const auto size = static_cast<glm::vec2>(texture.size());
      const auto size_constant = glm::vec4{size, 1.0f / size};

      constants.at(size_offset) = size_constant[0];
      constants.at(size_offset + 1) = size_constant[1];
      constants.at(size_offset + 2) = size_constant[2];
      constants.at(size_offset + 3) = size_constant[3];
   }

   return constants;
}
}

Material::Material(const Material_info& info, Com_ref<IDirect3DDevice9> device,
                   const Shader_rendertype_collection& rendertypes,
                   const Texture_database& textures) noexcept
   : _device{device},
     _overridden_rendertype{info.overridden_rendertype},
     _textures{resolve_textures(info, textures)},
     _constants{resolve_constants(info, _textures)},
     _rendertype{rendertypes.at(info.rendertype)}
{
}

auto Material::target_rendertype() const noexcept -> std::string_view
{
   return _overridden_rendertype;
}

void Material::bind() const noexcept
{
   for (auto i = 0u; i < _textures.size(); ++i) {
      _textures[i].bind(material_textures_offset + i);
   }

   _device->SetVertexShaderConstantF(sp::constants::vs::material_constants_start,
                                     _constants.data(), max_material_constants);

   _device->SetPixelShaderConstantF(sp::constants::ps::material_constants_start,
                                    _constants.data(), max_material_constants);
}

void Material::update(const std::string& shader_name,
                      const Vertex_shader_flags vertex_flags,
                      const Pixel_shader_flags pixel_flags) const noexcept
{
   if (const auto state = _rendertype.find(shader_name); state) {
      state->bind(*_device, vertex_flags, pixel_flags);
   }
   else {
      // TODO: Log attempt to use material on unsupported state here.

      _device->SetVertexShader(nullptr);
      _device->SetPixelShader(nullptr);
   }
}
}
