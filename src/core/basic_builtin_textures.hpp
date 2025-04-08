#pragma once

#include "texture_database.hpp"

#include <d3d11_4.h>

namespace sp::core {

struct Basic_builtin_textures {
   explicit Basic_builtin_textures(ID3D11Device5& device) noexcept;

   void add_to_database(Shader_resource_database& resources) noexcept;

   Com_ptr<ID3D11ShaderResourceView> white;
   Com_ptr<ID3D11ShaderResourceView> grey;
   Com_ptr<ID3D11ShaderResourceView> normal;
};

}