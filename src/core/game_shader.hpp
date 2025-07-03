#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"
#include "game_rendertypes.hpp"
#include "shader_input_layouts.hpp"

#include <array>
#include <exception>
#include <vector>

#include <d3d11_4.h>

namespace sp::core {

struct Game_shader {
   const Com_ptr<ID3D11VertexShader> vs;
   const Com_ptr<ID3D11VertexShader> vs_compressed;
   const Com_ptr<ID3D11PixelShader> ps;
   const Com_ptr<ID3D11PixelShader> ps_al;
   const Com_ptr<ID3D11PixelShader> ps_oit;
   const Com_ptr<ID3D11PixelShader> ps_al_oit;

   const bool light_active;
   const std::uint8_t light_active_point_count;
   const bool light_active_spot;

   const Rendertype rendertype;
   const std::array<bool, 4> srgb_state;
   const std::string shader_name;
   const shader::Vertex_shader_flags vertex_shader_flags;

   Shader_input_layouts input_layouts;
   Shader_input_layouts input_layouts_compressed;
};

class Game_shader_store {
public:
   explicit Game_shader_store(shader::Rendertypes_database& database) noexcept;

   auto operator[](const std::uint32_t index) noexcept -> Game_shader&
   {
      if (index >= _shaders.size()) std::terminate();

      return _shaders[index];
   }

private:
   std::vector<Game_shader> _shaders;
};

}
