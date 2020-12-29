#pragma once

#include "../shader/database.hpp"
#include "shader_set.hpp"

namespace sp::material {

class Shader_factory {
public:
   Shader_factory(Com_ptr<ID3D11Device5> device,
                  shader::Rendertypes_database& shaders) noexcept;

   auto create(const std::string& rendertype) noexcept -> std::shared_ptr<Shader_set>;

private:
   const Com_ptr<ID3D11Device5> _device;
   shader::Rendertypes_database& _shaders;

   std::unordered_map<std::string, std::shared_ptr<Shader_set>> _cache;
};

}
