#pragma once

#include "../../effects/cmaa2.hpp"
#include "../../effects/profiler.hpp"
#include "../../effects/rendertarget_allocator.hpp"
#include "../swapchain.hpp"
#include "random.hpp"

#include <array>

#include <d3d11_4.h>

namespace sp::core::postprocessing {

class Backbuffer_resolver {
public:
   Backbuffer_resolver(Com_ptr<ID3D11Device5> device, shader::Database& shaders);

   struct Input {
      ID3D11Texture2D& texture;
      ID3D11ShaderResourceView& srv;
      ID3D11UnorderedAccessView* uav = nullptr; // (optional) for CMAA2.

      DXGI_FORMAT format;
      UINT width;
      UINT height;
      UINT sample_count;
   };

   struct Flags {
      bool use_cmma2;
      bool linear_input;
   };

   struct Interfaces {
      effects::CMAA2& cmaa2;
      effects::Rendertarget_allocator& rt_allocator;
      effects::Profiler& profiler;
      const Shader_resource_database& resources;
   };

   void apply(ID3D11DeviceContext1& dc, const Input input, const Flags flags,
              Swapchain& swapchain, const Interfaces interfaces) noexcept;

private:
   void apply_matching_format(ID3D11DeviceContext1& dc, const Input& input,
                              const Flags flags, Swapchain& swapchain,
                              const Interfaces& interfaces) noexcept;

   void apply_mismatch_format(ID3D11DeviceContext1& dc, const Input& input,
                              const Flags flags, Swapchain& swapchain,
                              const Interfaces& interfaces) noexcept;

   void resolve_input(ID3D11DeviceContext1& dc, const Input& input,
                      const Flags flags, const Interfaces& interfaces,
                      ID3D11RenderTargetView& target) noexcept;

   auto get_blue_noise_texture(const Interfaces& interfaces) noexcept
      -> ID3D11ShaderResourceView*;

   auto get_resolve_pixel_shader(const Input& input, const Flags flags) noexcept
      -> ID3D11PixelShader*;

   const Com_ptr<ID3D11VertexShader> _resolve_vs;
   const Com_ptr<ID3D11PixelShader> _resolve_ps;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_x2;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_x4;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_x8;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_linear;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_linear_x2;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_linear_x4;
   const Com_ptr<ID3D11PixelShader> _resolve_ps_linear_x8;
   const Com_ptr<ID3D11Buffer> _resolve_cb;
   std::array<Com_ptr<ID3D11ShaderResourceView>, 64> _blue_noise_srvs;
   xor_shift32 _resolve_xorshift;
   std::uniform_int_distribution<xor_shift32::result_type> _resolve_rand_dist{0, 63};
};

}
