#pragma once

#include "com_ptr.hpp"
#include "game_rendertypes.hpp"

#include <array>

#include <d3d11_1.h>

namespace sp::core {

struct Game_shader {
   Com_ptr<ID3D11VertexShader> vs;
   Com_ptr<ID3D11VertexShader> vs_compressed;
   Com_ptr<ID3D11PixelShader> ps;

   Rendertype rendertype;
   std::array<bool, 4> srgb_state;
   std::string shader_name;
};

}
