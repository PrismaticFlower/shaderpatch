
#include "shader_factory.hpp"

namespace sp::material {

Shader_factory::Shader_factory(Com_ptr<ID3D11Device5> device,
                               shader::Rendertypes_database& shaders) noexcept
   : _device{std::move(device)}, _shaders{shaders}
{
}

auto Shader_factory::create(std::string rendertype, Flags flags) noexcept
   -> std::shared_ptr<Shader_set>
{
   if (auto it = _cache.find(std::tie(rendertype, flags)); it != _cache.end()) {
      return it->second;
   }

   auto shader_set = std::make_shared<Shader_set>(_device, _shaders[rendertype],
                                                  flags, rendertype);

   return _cache
      .emplace(std::tuple{std::move(rendertype), std::move(flags)}, std::move(shader_set))
      .first->second;
}

}
