
#include "late_backbuffer_resolver.hpp"

namespace sp::core {

Late_backbuffer_resolver::Late_backbuffer_resolver(const Shader_database& shader_database) noexcept
   : _vs{std::get<0>(shader_database.groups.at("late_backbuffer_resolve"s)
                        .vertex.at("main_vs"s)
                        .copy())},
     _ps{shader_database.groups.at("late_backbuffer_resolve"s)
            .pixel.at("main_ps"s)
            .copy()},
     _ps_ms{shader_database.groups.at("late_backbuffer_resolve"s)
               .pixel.at("main_ms_ps"s)
               .copy()},
     _ps_linear{shader_database.groups.at("late_backbuffer_resolve"s)
                   .pixel.at("main_linear_ps"s)
                   .copy()},
     _ps_linear_ms{shader_database.groups.at("late_backbuffer_resolve"s)
                      .pixel.at("main_linear_ms_ps"s)
                      .copy()}
{
}

void Late_backbuffer_resolver::resolve(ID3D11DeviceContext1& dc,
                                       const Game_rendertarget& source,
                                       const Swapchain& swapchain) const noexcept
{
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_vs.get(), nullptr, 0);

   const CD3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(swapchain.width()),
                                  static_cast<float>(swapchain.height())};
   auto* const srv = source.srv.get();
   auto* const rtv = swapchain.rtv();

   dc.RSSetViewports(1, &viewport);

   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetShader(get_pixel_shader(source), nullptr, 0);

   dc.Draw(3, 0);
}

auto Late_backbuffer_resolver::get_pixel_shader(const Game_rendertarget& source) const
   noexcept -> ID3D11PixelShader*
{
   if (source.format == DXGI_FORMAT_R32G32B32A32_FLOAT ||
       source.format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
       source.format == DXGI_FORMAT_R11G11B10_FLOAT) {
      return (source.sample_count == 1) ? _ps_linear.get() : _ps_linear_ms.get();
   }

   return (source.sample_count == 1) ? _ps.get() : _ps_ms.get();
}

}
