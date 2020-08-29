#pragma once

#include "com_ptr.hpp"
#include "game_rendertarget.hpp"
#include "random.hpp"
#include "shader_database.hpp"
#include "swapchain.hpp"
#include "texture_database.hpp"

#include <d3d11_1.h>

namespace sp::core {

class Late_backbuffer_resolver {
public:
   Late_backbuffer_resolver(ID3D11Device1& device,
                            const Shader_database& shader_database) noexcept;

   ~Late_backbuffer_resolver() = default;
   Late_backbuffer_resolver(const Late_backbuffer_resolver&) = default;
   Late_backbuffer_resolver& operator=(const Late_backbuffer_resolver&) = default;
   Late_backbuffer_resolver(Late_backbuffer_resolver&&) = default;
   Late_backbuffer_resolver& operator=(Late_backbuffer_resolver&&) = default;

   void resolve(ID3D11DeviceContext1& dc, const Shader_resource_database& textures,
                const bool linear_source, const Game_rendertarget& source,
                ID3D11RenderTargetView& target) noexcept;

private:
   void update_blue_noise_srv(const Shader_resource_database& textures);

   auto get_pixel_shader(const bool linear_source,
                         const Game_rendertarget& source) const noexcept
      -> ID3D11PixelShader*;

   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps;
   const Com_ptr<ID3D11PixelShader> _ps_x2;
   const Com_ptr<ID3D11PixelShader> _ps_x4;
   const Com_ptr<ID3D11PixelShader> _ps_x8;
   const Com_ptr<ID3D11PixelShader> _ps_linear;
   const Com_ptr<ID3D11PixelShader> _ps_linear_x2;
   const Com_ptr<ID3D11PixelShader> _ps_linear_x4;
   const Com_ptr<ID3D11PixelShader> _ps_linear_x8;
   const Com_ptr<ID3D11Buffer> _cb;
   Com_ptr<ID3D11ShaderResourceView> _blue_noise_srv;
   xor_shift32 _xorshift;
   std::uniform_int_distribution<xor_shift32::result_type> _rand_dist{0, 63};
};
}
