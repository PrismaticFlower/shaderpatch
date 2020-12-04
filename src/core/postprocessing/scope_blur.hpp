#pragma once

#include "../../effects/rendertarget_allocator.hpp"
#include "../../shader/database.hpp"
#include "../game_rendertarget.hpp"
#include "com_ptr.hpp"

#include <cstdint>

#include <glm/glm.hpp>

#include <d3d11_4.h>

namespace sp::core::postprocessing {

class Scope_blur {
public:
   Scope_blur(ID3D11Device5& device, shader::Database& shaders) noexcept;

   ~Scope_blur() = default;
   Scope_blur(const Scope_blur&) = default;
   Scope_blur& operator=(const Scope_blur&) = default;
   Scope_blur(Scope_blur&&) = default;
   Scope_blur& operator=(Scope_blur&&) = default;

   void apply(ID3D11DeviceContext4& dc, const Game_rendertarget& dest,
              ID3D11ShaderResourceView& input_srv,
              const std::uint32_t input_width, const std::uint32_t input_height,
              effects::Rendertarget_allocator& rt_allocator) const noexcept;

private:
   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps_blur;
   const Com_ptr<ID3D11PixelShader> _ps_overlay;
   const Com_ptr<ID3D11SamplerState> _sampler;
   const Com_ptr<ID3D11BlendState> _overlay_blend;
};

}
