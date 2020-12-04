#pragma once

#include "../../effects/rendertarget_allocator.hpp"
#include "../../shader/database.hpp"
#include "../game_rendertarget.hpp"
#include "com_ptr.hpp"

#include <cstdint>

#include <glm/glm.hpp>

#include <d3d11_4.h>

namespace sp::core::postprocessing {

class Scene_blur {
public:
   Scene_blur(ID3D11Device5& device, shader::Database& shaders) noexcept;

   ~Scene_blur() = default;
   Scene_blur(const Scene_blur&) = default;
   Scene_blur& operator=(const Scene_blur&) = default;
   Scene_blur(Scene_blur&&) = default;
   Scene_blur& operator=(Scene_blur&&) = default;

   void blur_factor(const float factor) noexcept;

   void apply(ID3D11DeviceContext4& dc, const Game_rendertarget& dest,
              ID3D11ShaderResourceView& input_srv,
              const std::uint32_t input_width, const std::uint32_t input_height,
              effects::Rendertarget_allocator& rt_allocator) const noexcept;

private:
   struct alignas(16) Input_vars {
      glm::vec2 blur_dir_size = {0.0f, 0.0f};
      float factor = 0.0f;
      std::uint32_t padding{};
   };

   static_assert(sizeof(Input_vars) == 16);

   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps_blur;
   const Com_ptr<ID3D11PixelShader> _ps_overlay;
   const Com_ptr<ID3D11Buffer> _constant_buffer;
   const Com_ptr<ID3D11SamplerState> _sampler;
   const Com_ptr<ID3D11BlendState> _overlay_blend;

   float _blur_factor = 1.0f;
};

}
