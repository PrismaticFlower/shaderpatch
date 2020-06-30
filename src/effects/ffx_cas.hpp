#pragma once

#include "../core/shader_database.hpp"
#include "com_ptr.hpp"
#include "postprocess_params.hpp"
#include "profiler.hpp"

#include <d3d11_1.h>

namespace sp::effects {

struct FFX_cas_inputs {
   const std::uint32_t width;
   const std::uint32_t height;
   ID3D11ShaderResourceView& input;
   ID3D11UnorderedAccessView& output;
};

class FFX_cas {
public:
   FFX_cas(Com_ptr<ID3D11Device1> device,
           const core::Shader_group_collection& shader_groups) noexcept;

   void apply(ID3D11DeviceContext1& dc, Profiler& profiler,
              const FFX_cas_inputs inputs) noexcept;

   void params(const FFX_cas_params& params) noexcept
   {
      _params = params;
   }

   auto params() const noexcept -> const FFX_cas_params&
   {
      return _params;
   }

private:
   struct Constants {
      glm::vec4 scaling;
      float sharpness;
      std::uint32_t fp16_sharpness;
      float scaled_x_pixel_size_8x;
      std::uint32_t padding;
   };

   static_assert(sizeof(Constants) == 32);

   static auto pack_constants(const float input_sharpness, const glm::vec2 input_size,
                              const glm::vec2 output_size) noexcept -> Constants;

   FFX_cas_params _params{};

   const bool _fp16_supported;

   const Com_ptr<ID3D11Buffer> _constant_buffer;
   const Com_ptr<ID3D11ComputeShader> _cs_sharpen_only;
   const Com_ptr<ID3D11ComputeShader> _cs_upscale;
   const Com_ptr<ID3D11ComputeShader> _cs_fp16_sharpen_only;
   const Com_ptr<ID3D11ComputeShader> _cs_fp16_upscale;
};

}
