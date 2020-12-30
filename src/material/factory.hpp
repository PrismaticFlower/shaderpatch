#pragma once

#include "../core/texture_database.hpp"
#include "com_ptr.hpp"
#include "material.hpp"
#include "patch_material_io.hpp"
#include "shader_factory.hpp"

#include <limits>
#include <utility>
#include <vector>

#include <d3d11_4.h>

namespace sp::material {

class Factory {
public:
   Factory(Com_ptr<ID3D11Device5> device,
           shader::Rendertypes_database& shader_rendertypes_database,
           core::Shader_resource_database& shader_resource_database)
      : _device{device},
        _shader_factory{device, shader_rendertypes_database},
        _shader_resource_database{shader_resource_database}
   {
   }

   auto create_material(const Material_config& config) noexcept -> material::Material;

private:
   Com_ptr<ID3D11Device5> _device;
   Shader_factory _shader_factory;
   core::Shader_resource_database& _shader_resource_database;
};

}
