
#include "ffx_cas.hpp"
#include "../core/d3d11_helpers.hpp"

#include <cmath>

namespace sp::effects {

namespace {

constexpr auto cas_fp16_flag = 0b1;
constexpr auto cas_sharpen_only_flag = 0b10;

bool fp16_supported(ID3D11Device1& device)
{
   D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT data{};
   device.CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &data,
                              sizeof(data));

   return (data.AllOtherShaderStagesMinPrecision &
           D3D11_SHADER_MIN_PRECISION_16_BIT) != 0;
}

}

FFX_cas::FFX_cas(Com_ptr<ID3D11Device1> device,
                 const core::Shader_group_collection& shader_groups) noexcept
   : _fp16_supported{fp16_supported(*device)},
     _constant_buffer{core::create_dynamic_constant_buffer(*device, sizeof(Constants))},
     _cs_sharpen_only{
        shader_groups.at("ffx_cas"s).compute.at("main_cs"s).copy(cas_sharpen_only_flag)},
     _cs_upscale{shader_groups.at("ffx_cas"s).compute.at("main_cs"s).copy(0b0)},
     _cs_fp16_sharpen_only{
        shader_groups.at("ffx_cas"s).compute.at("main_cs"s).copy_if(cas_fp16_flag | cas_sharpen_only_flag)},
     _cs_fp16_upscale{
        shader_groups.at("ffx_cas"s).compute.at("main_cs"s).copy_if(cas_fp16_flag)}
{
}

void FFX_cas::apply(ID3D11DeviceContext1& dc, Profiler& profiler,
                    const FFX_cas_inputs inputs) noexcept
{
   Profile profile{profiler, dc, "FFX Contrast Adaptive Sharpening"};

   const glm::vec2 flt_size{static_cast<float>(inputs.width),
                            static_cast<float>(inputs.height)};

   core::update_dynamic_buffer(dc, *_constant_buffer,
                               pack_constants(_params.sharpness, flt_size, flt_size));

   dc.ClearState();

   dc.CSSetShader((_fp16_supported ? _cs_fp16_sharpen_only : _cs_sharpen_only).get(),
                  nullptr, 0);

   auto* const buffer = _constant_buffer.get();
   dc.CSSetConstantBuffers(0, 1, &buffer);

   auto* const srv = &inputs.input;
   dc.CSSetShaderResources(0, 1, &srv);

   auto* const uav = &inputs.output;
   dc.CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

   dc.Dispatch((inputs.width + 15) / 16, (inputs.height + 15) / 16, 1);

   // Be polite, unbind the SRVs and UAVs.
   ID3D11ShaderResourceView* const null_srv = nullptr;
   dc.CSSetShaderResources(0, 1, &null_srv);
   ID3D11UnorderedAccessView* const null_uav = nullptr;
   dc.CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
}

auto FFX_cas::pack_constants(const float input_sharpness, const glm::vec2 input_size,
                             const glm::vec2 output_size) noexcept -> Constants
{
   Constants packed{};

   packed.scaling = {input_size / output_size, 0.5f * input_size / output_size - 0.5f};

   const float sharpness =
      -(1.0f / glm::mix(8.0f, 5.0f, glm::clamp(input_sharpness, 0.0f, 1.0f)));

   packed.sharpness = sharpness;
   packed.fp16_sharpness = glm::packHalf2x16({sharpness, 0.0f});
   packed.scaled_x_pixel_size_8x = 8.0f * input_size.x / output_size.x;

   return packed;
}

}
