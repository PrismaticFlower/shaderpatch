#pragma once

#include "../shader/database.hpp"
#include "material_shader.hpp"

namespace sp::core {

class Material_shader_factory {
public:
   Material_shader_factory(Com_ptr<ID3D11Device5> device,
                           shader::Rendertypes_database& shaders) noexcept;

   auto create(const std::string& rendertype) noexcept
      -> std::shared_ptr<Material_shader>;

private:
   const Com_ptr<ID3D11Device5> _device;
   shader::Rendertypes_database& _shaders;

   std::unordered_map<std::string, std::shared_ptr<Material_shader>> _cache;
};

}
