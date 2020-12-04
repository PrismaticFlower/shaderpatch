
#include "scene_blur.hpp"
#include "../d3d11_helpers.hpp"
#include "utility.hpp"

#include <array>

#include <gsl/gsl>

namespace sp::core::postprocessing {

Scene_blur::Scene_blur(ID3D11Device5& device, shader::Database& shaders) noexcept
   : _vs{std::get<0>(shaders.vertex("postprocess"sv).entrypoint("main_vs"sv))},
     _ps_blur{shaders.pixel("scene blur"sv).entrypoint("blur_ps"sv)},
     _ps_overlay{shaders.pixel("scene blur"sv).entrypoint("overlay_ps"sv)},
     _constant_buffer{create_dynamic_constant_buffer(device, sizeof(Input_vars))},
     _sampler{[&] {
        Com_ptr<ID3D11SamplerState> sampler;

        CD3D11_SAMPLER_DESC desc{CD3D11_DEFAULT{}};
        desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;

        device.CreateSamplerState(&desc, sampler.clear_and_assign());

        return sampler;
     }()},
     _overlay_blend{[&] {
        Com_ptr<ID3D11BlendState> blend;

        CD3D11_BLEND_DESC desc{CD3D11_DEFAULT{}};

        desc.RenderTarget[0] =
           D3D11_RENDER_TARGET_BLEND_DESC{.BlendEnable = true,
                                          .SrcBlend = D3D11_BLEND_SRC_ALPHA,
                                          .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                                          .BlendOp = D3D11_BLEND_OP_ADD,
                                          .SrcBlendAlpha = D3D11_BLEND_ONE,
                                          .DestBlendAlpha = D3D11_BLEND_ZERO,
                                          .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                                          .RenderTargetWriteMask = 0b111};

        device.CreateBlendState(&desc, blend.clear_and_assign());

        return blend;
     }()}
{
}

void Scene_blur::blur_factor(const float factor) noexcept
{
   _blur_factor = factor;
}

void Scene_blur::apply(ID3D11DeviceContext4& dc, const Game_rendertarget& dest,
                       ID3D11ShaderResourceView& input_srv,
                       const std::uint32_t input_width, const std::uint32_t input_height,
                       effects::Rendertarget_allocator& rt_allocator) const noexcept
{
   // Setup state
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   dc.VSSetShader(_vs.get(), nullptr, 0);

   auto* const samp = _sampler.get();
   dc.PSSetSamplers(0, 1, &samp);

   const auto x_rt =
      rt_allocator.allocate({.format = dest.format,
                             .width = input_width,
                             .height = input_height,
                             .bind_flags = effects::rendertarget_bind_srv_rtv});
   const auto y_rt =
      rt_allocator.allocate({.format = dest.format,
                             .width = input_width,
                             .height = input_height,
                             .bind_flags = effects::rendertarget_bind_srv_rtv});

   const CD3D11_VIEWPORT blur_viewport{0.0f, 0.0f, static_cast<float>(input_width),
                                       static_cast<float>(input_height)};
   dc.RSSetViewports(1, &blur_viewport);

   auto* const cb = _constant_buffer.get();
   dc.PSSetConstantBuffers(0, 1, &cb);
   dc.PSSetShader(_ps_blur.get(), nullptr, 0);

   // Blur X
   {
      update_dynamic_buffer(dc, *_constant_buffer,
                            Input_vars{.blur_dir_size = {1.0f / input_width, 0.0f}});

      auto* const srv = &input_srv;
      dc.PSSetShaderResources(0, 1, &srv);

      auto* const rtv = &x_rt.rtv();
      dc.OMSetRenderTargets(1, &rtv, nullptr);

      dc.Draw(3, 0);
   }

   update_dynamic_buffer(dc, *_constant_buffer,
                         Input_vars{.blur_dir_size = {0.0f, 1.0f / input_height},
                                    .factor = _blur_factor});

   // Blur Y
   {
      dc.OMSetRenderTargets(0, nullptr, nullptr);

      auto* const srv = &x_rt.srv();
      dc.PSSetShaderResources(0, 1, &srv);

      auto* const rtv = &y_rt.rtv();
      dc.OMSetRenderTargets(1, &rtv, nullptr);

      dc.Draw(3, 0);
   }

   // Overlay
   {
      const CD3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(dest.width),
                                     static_cast<float>(dest.height)};
      dc.RSSetViewports(1, &viewport);

      dc.PSSetShader(_ps_overlay.get(), nullptr, 0);

      dc.OMSetRenderTargets(0, nullptr, nullptr);

      auto* const srv = &y_rt.srv();
      dc.PSSetShaderResources(0, 1, &srv);

      auto* const rtv = dest.rtv.get();
      dc.OMSetRenderTargets(1, &rtv, nullptr);
      dc.OMSetBlendState(_overlay_blend.get(), nullptr, 0xffffffffu);

      dc.Draw(3, 0);
   }
}

}
