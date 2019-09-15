
#include "bloom.hpp"
#include "../d3d11_helpers.hpp"
#include "utility.hpp"

#include <array>
#include <cmath>

#include <gsl/gsl>

namespace sp::core::postprocessing {

namespace {
const auto threshold_scoped_flag = 0b1;
}

Bloom::Bloom(ID3D11Device5& device, const Shader_database& shader_database) noexcept
   : _vs{std::get<0>(
        shader_database.groups.at("postprocess"s).vertex.at("main_vs"s).copy())},
     _ps_threshold{
        shader_database.groups.at("bloom alt"s).pixel.at("threshold_ps"s).copy()},
     _ps_scoped_threshold{
        shader_database.groups.at("bloom alt"s).pixel.at("threshold_ps"s).copy(threshold_scoped_flag)},
     _ps_overlay{shader_database.groups.at("bloom alt"s).pixel.at("overlay_ps"s).copy()},
     _ps_brighten{
        shader_database.groups.at("bloom alt"s).pixel.at("brighten_ps"s).copy()},
     _ps_199_blur{shader_database.groups.at("gaussian blur"s)
                     .pixel.at("gaussian_blur_199_ps"s)
                     .copy()},
     _ps_151_blur{shader_database.groups.at("gaussian blur"s)
                     .pixel.at("gaussian_blur_151_ps"s)
                     .copy()},
     _ps_99_blur{shader_database.groups.at("gaussian blur"s)
                    .pixel.at("gaussian_blur_99_ps"s)
                    .copy()},
     _ps_75_blur{shader_database.groups.at("gaussian blur"s)
                    .pixel.at("gaussian_blur_75_ps"s)
                    .copy()},
     _ps_51_blur{shader_database.groups.at("gaussian blur"s)
                    .pixel.at("gaussian_blur_51_ps"s)
                    .copy()},
     _ps_39_blur{shader_database.groups.at("gaussian blur"s)
                    .pixel.at("gaussian_blur_39_ps"s)
                    .copy()},
     _ps_23_blur{shader_database.groups.at("gaussian blur"s)
                    .pixel.at("gaussian_blur_23_ps"s)
                    .copy()},
     _constant_buffer{create_dynamic_constant_buffer(device, sizeof(Input_vars))},
     _blur_constant_buffer{
        create_dynamic_constant_buffer(device, sizeof(Blur_input_vars))},
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
                                          .SrcBlend = D3D11_BLEND_ONE,
                                          .DestBlend = D3D11_BLEND_ONE,
                                          .BlendOp = D3D11_BLEND_OP_ADD,
                                          .SrcBlendAlpha = D3D11_BLEND_ONE,
                                          .DestBlendAlpha = D3D11_BLEND_ZERO,
                                          .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                                          .RenderTargetWriteMask = 0b111};

        device.CreateBlendState(&desc, blend.clear_and_assign());

        return blend;
     }()},
     _brighten_blend{[&] {
        Com_ptr<ID3D11BlendState> blend;

        CD3D11_BLEND_DESC desc{CD3D11_DEFAULT{}};

        desc.RenderTarget[0] =
           D3D11_RENDER_TARGET_BLEND_DESC{.BlendEnable = true,
                                          .SrcBlend = D3D11_BLEND_DEST_COLOR,
                                          .DestBlend = D3D11_BLEND_ONE,
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

void Bloom::threshold(const float threshold) noexcept
{
   _glow_threshold = threshold;
}

void Bloom::intensity(const float intensity) noexcept
{
   _intensity = intensity;
}

void Bloom::apply(ID3D11DeviceContext4& dc, const Game_rendertarget& dest,
                  ID3D11ShaderResourceView& input_srv,
                  effects::Rendertarget_allocator& rt_allocator) const noexcept
{
   // Setup state
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   dc.VSSetShader(_vs.get(), nullptr, 0);

   auto* const samp = _sampler.get();
   dc.PSSetSamplers(0, 1, &samp);

   const auto process_width = dest.width / 4;
   const auto process_height = dest.height / 4;
   const auto x_rt = rt_allocator.allocate(DXGI_FORMAT_R11G11B10_FLOAT,
                                           process_width, process_height);
   const auto y_rt = rt_allocator.allocate(DXGI_FORMAT_R11G11B10_FLOAT,
                                           process_width, process_height);

   update_dynamic_buffer(dc, *_constant_buffer,
                         Input_vars{.threshold = _glow_threshold,
                                    .intensity = _intensity});

   const CD3D11_VIEWPORT threshold_blur_viewport{0.0f, 0.0f,
                                                 static_cast<float>(process_width),
                                                 static_cast<float>(process_height)};
   dc.RSSetViewports(1, &threshold_blur_viewport);

   // Threshold
   {
      auto* const cb = _constant_buffer.get();
      dc.PSSetConstantBuffers(0, 1, &cb);
      dc.PSSetShader(_scope_blur ? _ps_scoped_threshold.get() : _ps_threshold.get(),
                     nullptr, 0);

      auto* const srv = &input_srv;
      dc.PSSetShaderResources(0, 1, &srv);

      auto* const rtv = &y_rt.rtv();
      dc.OMSetRenderTargets(1, &rtv, nullptr);

      dc.Draw(3, 0);
   }

   // Blur
   {
      auto* const blur_cb = _blur_constant_buffer.get();
      dc.PSSetConstantBuffers(0, 1, &blur_cb);
      dc.PSSetShader(select_blur_shader(dest), nullptr, 0);

      // Blur X
      {
         update_dynamic_buffer(dc, *_blur_constant_buffer,
                               Blur_input_vars{
                                  .blur_dir_size = {1.0f / process_width, 0.0f}});

         dc.OMSetRenderTargets(0, nullptr, nullptr);

         auto* const srv = &y_rt.srv();
         dc.PSSetShaderResources(0, 1, &srv);

         auto* const rtv = &x_rt.rtv();
         dc.OMSetRenderTargets(1, &rtv, nullptr);

         dc.Draw(3, 0);
      }

      // Blur Y
      {
         update_dynamic_buffer(dc, *_blur_constant_buffer,
                               Blur_input_vars{
                                  .blur_dir_size = {0.0f, 1.0f / process_height}});

         dc.OMSetRenderTargets(0, nullptr, nullptr);

         auto* const srv = &x_rt.srv();
         dc.PSSetShaderResources(0, 1, &srv);

         auto* const rtv = &y_rt.rtv();
         dc.OMSetRenderTargets(1, &rtv, nullptr);

         dc.Draw(3, 0);
      }
   }

   const CD3D11_VIEWPORT overlay_bright_viewport{0.0f, 0.0f,
                                                 static_cast<float>(dest.width),
                                                 static_cast<float>(dest.height)};
   dc.RSSetViewports(1, &overlay_bright_viewport);

   auto* const cb = _constant_buffer.get();
   dc.PSSetConstantBuffers(0, 1, &cb);

   // Brighten
   {
      dc.OMSetRenderTargets(0, nullptr, nullptr);

      dc.PSSetShader(_ps_brighten.get(), nullptr, 0);

      ID3D11ShaderResourceView* const null_srv = nullptr;
      dc.PSSetShaderResources(0, 1, &null_srv);

      auto* const rtv = dest.rtv.get();
      dc.OMSetRenderTargets(1, &rtv, nullptr);
      dc.OMSetBlendState(_brighten_blend.get(), nullptr, 0xffffffffu);

      dc.Draw(3, 0);
   }

   // Overlay
   {
      dc.PSSetShader(_ps_overlay.get(), nullptr, 0);

      auto* const srv = &y_rt.srv();
      dc.PSSetShaderResources(0, 1, &srv);

      auto* const rtv = dest.rtv.get();
      dc.OMSetRenderTargets(1, &rtv, nullptr);
      dc.OMSetBlendState(_overlay_blend.get(), nullptr, 0xffffffffu);

      dc.Draw(3, 0);
   }
}

#pragma optimize("", off)

auto Bloom::select_blur_shader(const Game_rendertarget& dest) const noexcept
   -> ID3D11PixelShader*
{
   const auto length = safe_max(dest.width, dest.height) / 4;

   if (length >= 1280) return _ps_199_blur.get();
   if (length >= 960) return _ps_151_blur.get();
   if (length >= 640) return _ps_99_blur.get();
   if (length >= 480) return _ps_75_blur.get();
   if (length >= 320) return _ps_51_blur.get();
   if (length >= 256) return _ps_39_blur.get();

   return _ps_23_blur.get();

   // constexpr std::array target_resolutions{1280, 960, 640, 480, 320, 256,
   // 160}; const std::array shaders{_ps_199_blur.get(), _ps_151_blur.get(),
   //                          _ps_99_blur.get(),  _ps_75_blur.get(),
   //                          _ps_51_blur.get(),  _ps_39_blur.get(),
   //                          _ps_23_blur.get()};
   // constexpr auto shader_count = shaders.size();
   //
   // static_assert(target_resolutions.size() == shaders.size());
   //
   // const std::array<int, shader_count> length_distances = [&] {
   //    std::array<int, shader_count> length_distances;
   //
   //    std::iota(length_distances.begin(), length_distances.end(), 0);
   //
   //    for (auto& i : length_distances) {
   //       i = std::abs(target_resolutions[i] - length);
   //    }
   //
   //    return length_distances;
   // }();
   //
   // std::array<int, shader_count> indices;
   // std::iota(indices.begin(), indices.end(), 0);
   //
   // std::sort(indices.begin(), indices.end(), [&](const int l, const int r) {
   //    return length_distances[l] < length_distances[r];
   // });
   //
   // return shaders[indices[0]];
}

}
