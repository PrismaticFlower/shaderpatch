
#include "material_shader_factory.hpp"

namespace sp::core {

Material_shader_factory::Material_shader_factory(Com_ptr<ID3D11Device5> device,
                                                 shader::Rendertypes_database& shaders) noexcept
   : _device{std::move(device)}, _shaders{shaders}
{
}

auto Material_shader_factory::create(const std::string& rendertype) noexcept
   -> std::shared_ptr<Material_shader>
{
   if (auto it = _cache.find(rendertype); it != _cache.end()) return it->second;

   return _cache
      .emplace(rendertype,
               std::make_shared<Material_shader>(_device, _shaders[rendertype], rendertype))
      .first->second;
}

}
