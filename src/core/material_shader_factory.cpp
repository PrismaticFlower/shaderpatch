
#include "material_shader_factory.hpp"

namespace sp::core {

Material_shader_factory::Material_shader_factory(Com_ptr<ID3D11Device1> device,
                                                 std::shared_ptr<Shader_database> shader_database) noexcept
   : _device{std::move(device)}, _shader_database{std::move(shader_database)}
{
}

auto Material_shader_factory::create(const std::string& rendertype) noexcept
   -> std::shared_ptr<Material_shader>
{
   if (auto it = _cache.find(rendertype); it != _cache.end()) return it->second;

   return _cache
      .emplace(rendertype,
               std::make_shared<Material_shader>(_device,
                                                 _shader_database->rendertypes.at(rendertype),
                                                 rendertype))
      .first->second;
}

}
