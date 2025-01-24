#pragma once

#include "com_ptr.hpp"

#include <d3d11_2.h>

namespace sp::shader {

class Database;

}

namespace sp::shadows {

struct Draw_resources {
   Draw_resources(ID3D11Device& device, shader::Database& shaders);

   void update(ID3D11Device& device, INT depth_bias, float depth_bias_clamp,
               float slope_scaled_depth_bias) noexcept;

   Com_ptr<ID3D11Buffer> view_constants;

   Com_ptr<ID3D11InputLayout> input_layout;
   Com_ptr<ID3D11InputLayout> input_layout_textured;

   Com_ptr<ID3D11VertexShader> vertex_shader;
   Com_ptr<ID3D11VertexShader> vertex_shader_textured;

   Com_ptr<ID3D11RasterizerState> rasterizer_state;
   Com_ptr<ID3D11RasterizerState> rasterizer_state_doublesided;

   Com_ptr<ID3D11DepthStencilState> depth_stencil_state;

   Com_ptr<ID3D11PixelShader> pixel_shader_hardedged;

   Com_ptr<ID3D11SamplerState> sampler_state;

private:
   INT _depth_bias = 0;
   float _depth_bias_clamp = 0.0f;
   float _slope_scaled_depth_bias = 0.0f;
};

}