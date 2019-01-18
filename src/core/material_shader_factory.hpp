#pragma once

#include "material_shader.hpp"
#include "shader_database.hpp"

namespace sp::core {

class Material_shader_factory {
public:
   Material_shader_factory(Com_ptr<ID3D11Device1> device,
                           std::shared_ptr<Shader_database> shader_database) noexcept;

   auto create(const std::string& rendertype) noexcept
      -> std::shared_ptr<Material_shader>;

private:
   const Com_ptr<ID3D11Device1> _device;
   const std::shared_ptr<Shader_database> _shader_database;

   std::unordered_map<std::string, std::shared_ptr<Material_shader>> _cache;
};

}
