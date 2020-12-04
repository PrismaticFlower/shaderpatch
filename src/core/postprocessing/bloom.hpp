#pragma once

#include "../../effects/rendertarget_allocator.hpp"
#include "../../shader/database.hpp"
#include "../game_rendertarget.hpp"
#include "com_ptr.hpp"

#include <cstdint>

#include <glm/glm.hpp>

#include <d3d11_4.h>

namespace sp::core::postprocessing {

class Bloom {
public:
   Bloom(ID3D11Device5& device, shader::Database& shaders) noexcept;

   ~Bloom() = default;
   Bloom(const Bloom&) = default;
   Bloom& operator=(const Bloom&) = default;
   Bloom(Bloom&&) = default;
   Bloom& operator=(Bloom&&) = default;

   void apply(ID3D11DeviceContext4& dc, const Game_rendertarget& dest,
              ID3D11ShaderResourceView& input_srv,
              effects::Rendertarget_allocator& rt_allocator) const noexcept;

   void threshold(const float threshold) noexcept;

   void intensity(const float intensity) noexcept;

private:
   auto select_blur_shader(const Game_rendertarget& dest) const noexcept
      -> ID3D11PixelShader*;

   struct alignas(16) Input_vars {
      float threshold = 0.5f;
      float intensity = 1.0f;
      std::array<std::uint32_t, 2> padding{};
   };

   static_assert(sizeof(Input_vars) == 16);

   struct alignas(16) Blur_input_vars {
      glm::vec2 blur_dir_size = {0.0f, 0.0f};
      std::array<std::uint32_t, 2> padding{};
   };

   static_assert(sizeof(Blur_input_vars) == 16);

   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps_threshold;
   const Com_ptr<ID3D11PixelShader> _ps_scoped_threshold;
   const Com_ptr<ID3D11PixelShader> _ps_overlay;
   const Com_ptr<ID3D11PixelShader> _ps_brighten;
   const Com_ptr<ID3D11PixelShader> _ps_199_blur;
   const Com_ptr<ID3D11PixelShader> _ps_151_blur;
   const Com_ptr<ID3D11PixelShader> _ps_99_blur;
   const Com_ptr<ID3D11PixelShader> _ps_75_blur;
   const Com_ptr<ID3D11PixelShader> _ps_51_blur;
   const Com_ptr<ID3D11PixelShader> _ps_39_blur;
   const Com_ptr<ID3D11PixelShader> _ps_23_blur;
   const Com_ptr<ID3D11Buffer> _constant_buffer;
   const Com_ptr<ID3D11Buffer> _blur_constant_buffer;
   const Com_ptr<ID3D11SamplerState> _sampler;
   const Com_ptr<ID3D11BlendState> _overlay_blend;
   const Com_ptr<ID3D11BlendState> _brighten_blend;

   bool _scope_blur = false;
   float _glow_threshold = 0.5f;
   float _intensity = 1.0f;
};

}
