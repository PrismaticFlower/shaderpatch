#pragma once

#include "com_ptr.hpp"
#include "game_shader.hpp"
#include "shader_database.hpp"
#include "shader_input_layouts.hpp"

#include <unordered_map>

#include <d3d11_1.h>

namespace sp::core {

class Material_shader {
public:
   Material_shader(Com_ptr<ID3D11Device1> device,
                   const Shader_rendertype& rendertype) noexcept;

   void update(ID3D11DeviceContext1& dc,
               const Input_layout_descriptions& layout_descriptions,
               const std::uint16_t layout_index, const std::string& shader_name,
               const Vertex_shader_flags vertex_shader_flags,
               const Pixel_shader_flags pixel_shader_flags) noexcept;

private:
   struct Material_vertex_shader {
      Com_ptr<ID3D11VertexShader> vs;

      Shader_input_layouts input_layouts;
   };

   using State =
      std::pair<std::unordered_map<Vertex_shader_flags, Material_vertex_shader>,
                std::unordered_map<Pixel_shader_flags, Com_ptr<ID3D11PixelShader>>>;

   using Shaders = std::unordered_map<std::string, State>;

   static auto init_shaders(const Shader_rendertype& rendertype) noexcept -> Shaders;

   static auto init_state(const Shader_state& state) noexcept -> State;

   static auto init_vs_entrypoint(const Vertex_shader_entrypoint& vs,
                                  const std::uint16_t static_flags) noexcept
      -> std::unordered_map<Vertex_shader_flags, Material_vertex_shader>;

   static auto init_ps_entrypoint(const Pixel_shader_entrypoint& ps,
                                  const std::uint16_t static_flags) noexcept
      -> std::unordered_map<Pixel_shader_flags, Com_ptr<ID3D11PixelShader>>;

   const Com_ptr<ID3D11Device1> _device;

   Shaders _shaders;
};

}
