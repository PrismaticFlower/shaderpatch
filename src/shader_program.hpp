#pragma once

#include "com_ptr.hpp"

#include <utility>

#include <d3d9.h>

namespace sp {

class Shader_program {
public:
   Shader_program() = default;

   Shader_program(Com_ptr<IDirect3DVertexShader9> vertex_shader,
                  Com_ptr<IDirect3DPixelShader9> pixel_shader) noexcept
      : _vertex_shader{std::move(vertex_shader)}, _pixel_shader{std::move(pixel_shader)}
   {
   }

   void bind(IDirect3DDevice9& device) const noexcept
   {
      device.SetVertexShader(_vertex_shader.get());
      device.SetPixelShader(_pixel_shader.get());
   }

   auto get() const noexcept
      -> std::pair<IDirect3DVertexShader9*, IDirect3DPixelShader9*>
   {
      return {_vertex_shader.get(), _pixel_shader.get()};
   }

   auto get_vertex_shader() const noexcept -> IDirect3DVertexShader9*
   {
      return _vertex_shader.get();
   }

   auto get_pixel_shader() const noexcept -> IDirect3DPixelShader9*
   {
      return _pixel_shader.get();
   }

private:
   Com_ptr<IDirect3DVertexShader9> _vertex_shader;
   Com_ptr<IDirect3DPixelShader9> _pixel_shader;
};
}
