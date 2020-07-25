#pragma once

#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "shader_flags.hpp"
#include "shader_input_layouts.hpp"

#include <array>

#include <d3d11_1.h>

namespace sp::core {

struct Game_shader {
   const Com_ptr<ID3D11VertexShader> vs;
   const Com_ptr<ID3D11VertexShader> vs_compressed;
   const Com_ptr<ID3D11PixelShader> ps;
   const Com_ptr<ID3D11PixelShader> ps_oit;

   const bool light_active;
   const std::uint8_t light_active_point_count;
   const bool light_active_spot;

   const Rendertype rendertype;
   const std::array<bool, 4> srgb_state;
   const std::string shader_name;
   const Vertex_shader_flags vertex_shader_flags;

   Shader_input_layouts input_layouts;
   Shader_input_layouts input_layouts_compressed;
};

}
