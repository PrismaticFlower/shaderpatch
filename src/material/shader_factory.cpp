
#include "shader_factory.hpp"

namespace sp::material {

Shader_factory::Shader_factory(Com_ptr<ID3D11Device5> device,
                               shader::Rendertypes_database& shaders) noexcept
   : _device{std::move(device)}, _shaders{shaders}
{
}

auto Shader_factory::create(const std::string& rendertype) noexcept
   -> std::shared_ptr<Shader_set>
{
   if (auto it = _cache.find(rendertype); it != _cache.end()) return it->second;

   return _cache
      .emplace(rendertype,
               std::make_shared<Shader_set>(_device, _shaders[rendertype], rendertype))
      .first->second;
}

}
