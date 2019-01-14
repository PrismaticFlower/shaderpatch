#pragma once

#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "patch_material_io.hpp"
#include "shader_database.hpp"
#include "texture_database.hpp"

#include <vector>

#include <d3d11_1.h>

namespace sp::core {

struct Patch_material {
   Patch_material() = default;

   Patch_material(Material_info material_info,
                  const Shader_rendertype_collection& rendertypes,
                  const Texture_database& texture_database,
                  ID3D11Device1& device) noexcept;

   Rendertype overridden_rendertype;
   Shader_rendertype shaders;
   Com_ptr<ID3D11Buffer> constant_buffer;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> shader_resources;

   std::string name;
};

}
