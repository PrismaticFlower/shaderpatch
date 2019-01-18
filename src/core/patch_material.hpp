#pragma once

#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "game_texture.hpp"
#include "material_shader_factory.hpp"
#include "patch_material_io.hpp"
#include "texture_database.hpp"

#include <memory>
#include <vector>

#include <d3d11_1.h>

namespace sp::core {

struct Patch_material {
   Patch_material() = default;

   Patch_material(Material_info material_info, Material_shader_factory& shader_factory,
                  const Texture_database& texture_database,
                  ID3D11Device1& device) noexcept;

   Rendertype overridden_rendertype;
   std::shared_ptr<Material_shader> shader;
   Com_ptr<ID3D11Buffer> constant_buffer;
   std::vector<Com_ptr<ID3D11ShaderResourceView>> shader_resources;
   Game_texture fail_safe_game_texture;

   std::string name;
};

}
