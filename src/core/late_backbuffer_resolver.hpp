#pragma once

#include "com_ptr.hpp"
#include "game_rendertarget.hpp"
#include "shader_database.hpp"
#include "swapchain.hpp"

#include <d3d11_1.h>

namespace sp::core {

class Late_backbuffer_resolver {
public:
   Late_backbuffer_resolver(const Shader_database& shader_database) noexcept;

   ~Late_backbuffer_resolver() = default;
   Late_backbuffer_resolver(const Late_backbuffer_resolver&) = default;
   Late_backbuffer_resolver& operator=(const Late_backbuffer_resolver&) = default;
   Late_backbuffer_resolver(Late_backbuffer_resolver&&) = default;
   Late_backbuffer_resolver& operator=(Late_backbuffer_resolver&&) = default;

   void resolve(ID3D11DeviceContext1& dc, const Game_rendertarget& source,
                const Swapchain& swapchain) const noexcept;

private:
   auto get_pixel_shader(const Game_rendertarget& source) const noexcept
      -> ID3D11PixelShader*;

   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps;
   const Com_ptr<ID3D11PixelShader> _ps_ms;
   const Com_ptr<ID3D11PixelShader> _ps_linear;
   const Com_ptr<ID3D11PixelShader> _ps_linear_ms;
};

}
