
#include "backbuffer_resolver.hpp"
#include "../d3d11_helpers.hpp"

#include <DirectXTex.h>

namespace sp::core::postprocessing {

namespace {

struct Resolve_constants {
   float width;
   float height;
   std::array<std::uint32_t, 2> randomness;
};

static_assert(sizeof(Resolve_constants) == 16);

}

Backbuffer_resolver::Backbuffer_resolver(Com_ptr<ID3D11Device5> device,
                                         shader::Database& shaders)
   : _resolve_vs{std::get<0>(
        shaders.vertex("late_backbuffer_resolve"sv).entrypoint("main_vs"sv))},
     _resolve_ps{shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_ps"sv)},
     _resolve_ps_x2{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_x2_ps"sv)},
     _resolve_ps_x4{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_x4_ps"sv)},
     _resolve_ps_x8{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_x8_ps"sv)},
     _resolve_ps_linear{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_linear_ps"sv)},
     _resolve_ps_linear_x2{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_linear_x2_ps"sv)},
     _resolve_ps_linear_x4{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_linear_x4_ps"sv)},
     _resolve_ps_linear_x8{
        shaders.pixel("late_backbuffer_resolve"sv).entrypoint("main_linear_x8_ps"sv)},
     _resolve_cb{create_dynamic_constant_buffer(*device, sizeof(Resolve_constants))}
{
}

void Backbuffer_resolver::apply(ID3D11DeviceContext1& dc, const Input input,
                                const Flags flags, Swapchain& swapchain,
                                const Interfaces interfaces) noexcept
{
   if (DirectX::MakeTypeless(input.format) == DirectX::MakeTypeless(Swapchain::format)) {
      apply_matching_format(dc, input, flags, swapchain, interfaces);
   }
   else {
      apply_mismatch_format(dc, input, flags, swapchain, interfaces);
   }
}

void Backbuffer_resolver::apply_matching_format(ID3D11DeviceContext1& dc,
                                                const Input& input, const Flags flags,
                                                Swapchain& swapchain,
                                                const Interfaces& interfaces) noexcept
{
   const bool msaa_input = input.sample_count > 1;
   const bool want_post_aa = flags.use_cmma2;
   const bool unscaled =
      input.width == swapchain.width() && input.height == swapchain.height();

   if (msaa_input && !want_post_aa) {
      if (unscaled) {
         dc.ResolveSubresource(swapchain.texture(), 0, &input.texture, 0, input.format);

         return;
      }
      else {
         resolve_input(dc, input, flags, interfaces, *swapchain.rtv(),
                       (swapchain.width() - input.width) / 2.0f,
                       (swapchain.height() - input.height) / 2.0f, input.width,
                       input.height);

         return;
      }
   }

   if (flags.use_cmma2) {
      interfaces.cmaa2.apply(dc, interfaces.profiler,
                             {.input = input.srv,
                              .output = *input.uav,
                              .width = input.width,
                              .height = input.height});
   }

   const UINT swapchain_offset_left = (swapchain.width() - input.width) / 2;
   const UINT swapchain_offset_top = (swapchain.height() - input.height) / 2;

   dc.CopySubresourceRegion(swapchain.texture(), 0, swapchain_offset_left,
                            swapchain_offset_top, 0, &input.texture, 0, nullptr);
}

void Backbuffer_resolver::apply_mismatch_format(ID3D11DeviceContext1& dc,
                                                const Input& input, const Flags flags,
                                                Swapchain& swapchain,
                                                const Interfaces& interfaces) noexcept
{
   [[maybe_unused]] const bool msaa_input = input.sample_count > 1;
   const bool want_post_aa = flags.use_cmma2;

   // Fast Path for no post AA.
   if (!want_post_aa) {
      resolve_input(dc, input, flags, interfaces, *swapchain.rtv(),
                    (swapchain.width() - input.width) / 2.0f,
                    (swapchain.height() - input.height) / 2.0f, input.width,
                    input.height);

      return;
   }

   if (flags.use_cmma2) {
      auto cmma_target = interfaces.rt_allocator.allocate(
         {.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
          .width = input.width,
          .height = input.height,
          .bind_flags = effects::rendertarget_bind_srv_rtv_uav,
          .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      resolve_input(dc, input, flags, interfaces, *cmma_target.rtv(), 0.0f,
                    0.0f, input.width, input.height);

      interfaces.cmaa2.apply(dc, interfaces.profiler,
                             {.input = *cmma_target.srv(),
                              .output = *cmma_target.uav(),
                              .width = input.width,
                              .height = input.height});

      const UINT swapchain_offset_left = (swapchain.width() - input.width) / 2;
      const UINT swapchain_offset_top = (swapchain.height() - input.height) / 2;

      dc.CopySubresourceRegion(swapchain.texture(), 0, swapchain_offset_left,
                               swapchain_offset_top, 0, &cmma_target.texture(),
                               0, nullptr);

      return;
   }

   log_and_terminate("Failed to resolve backbuffer! This should never happen.");
}

void Backbuffer_resolver::resolve_input(
   ID3D11DeviceContext1& dc, const Input& input, const Flags flags,
   const Interfaces& interfaces, ID3D11RenderTargetView& target,
   const float target_offset_left, const float target_offset_top,
   const UINT target_width, const UINT target_height) noexcept
{
   effects::Profile profile{interfaces.profiler, dc, "Backbuffer Resolve"};

   const Resolve_constants constants{.width = static_cast<float>(input.width),
                                     .height = static_cast<float>(input.height),
                                     .randomness = {_resolve_rand_dist(_resolve_xorshift),
                                                    _resolve_rand_dist(_resolve_xorshift)}};

   update_dynamic_buffer(dc, *_resolve_cb, constants);

   auto* const cb = _resolve_cb.get();

   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_resolve_vs.get(), nullptr, 0);
   dc.VSSetConstantBuffers(0, 1, &cb);

   const D3D11_VIEWPORT viewport{.TopLeftX = target_offset_left,
                                 .TopLeftY = target_offset_top,
                                 .Width = static_cast<float>(target_width),
                                 .Height = static_cast<float>(target_height),
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f};
   dc.RSSetViewports(1, &viewport);

   auto* const rtv = &target;
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   const std::array srvs{&input.srv, get_blue_noise_texture(interfaces)};
   dc.PSSetShaderResources(0, srvs.size(), srvs.data());

   dc.PSSetConstantBuffers(0, 1, &cb);

   dc.PSSetShader(get_resolve_pixel_shader(input, flags), nullptr, 0);

   dc.Draw(3, 0);
}

auto Backbuffer_resolver::get_blue_noise_texture(const Interfaces& interfaces) noexcept
   -> ID3D11ShaderResourceView*
{
   if (_blue_noise_srvs[0] == nullptr) {
      for (int i = 0; i < 64; ++i) {
         _blue_noise_srvs[i] = interfaces.resources.at_if(
            "_SP_BUILTIN_blue_noise_rgb_"s + std::to_string(i));
      }
   }

   return _blue_noise_srvs[_resolve_rand_dist(_resolve_xorshift)].get();
}

auto Backbuffer_resolver::get_resolve_pixel_shader(const Input& input,
                                                   const Flags flags) noexcept
   -> ID3D11PixelShader*
{
   if (flags.linear_input) {
      switch (input.sample_count) {
      case 1:
         return _resolve_ps_linear.get();
      case 2:
         return _resolve_ps_linear_x2.get();
      case 4:
         return _resolve_ps_linear_x4.get();
      case 8:
         return _resolve_ps_linear_x8.get();
      }
   }

   switch (input.sample_count) {
   case 1:
      return _resolve_ps.get();
   case 2:
      return _resolve_ps_x2.get();
   case 4:
      return _resolve_ps_x4.get();
   case 8:
      return _resolve_ps_x8.get();
   default:
      log_and_terminate("Unsupported sample count!");
   }
}

}