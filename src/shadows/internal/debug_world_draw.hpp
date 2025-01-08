#pragma once

#include "world/game_model.hpp"
#include "world/model.hpp"
#include "world/object_instance.hpp"

#include <span>

#include <d3d11_2.h>

namespace sp::shadows {

struct Debug_world_draw {
   struct Inputs {
      std::span<const Model> models;
      std::span<const Game_model> game_models;
      std::span<const Object_instance> object_instances;
   };

   Debug_world_draw(ID3D11Device& device);

   void draw(ID3D11DeviceContext2& dc, const Inputs& world,
             const float camera_yaw, const float camera_pitch,
             const glm::vec3& camera_positioWS, ID3D11Buffer* index_buffer,
             ID3D11Buffer* vertex_buffer, const D3D11_VIEWPORT& viewport,
             ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv) noexcept;

private:
   Com_ptr<ID3D11Device> _device;

   Com_ptr<ID3D11Buffer> _constants;

   Com_ptr<ID3D11InputLayout> _input_layout;
   Com_ptr<ID3D11VertexShader> _vertex_shader;

   Com_ptr<ID3D11RasterizerState> _rasterizer_state;
   Com_ptr<ID3D11RasterizerState> _doublesided_rasterizer_state;

   Com_ptr<ID3D11DepthStencilState> _depth_stencil_state;

   Com_ptr<ID3D11PixelShader> _pixel_shader;

   void initialize() noexcept;
};

}