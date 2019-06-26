
#include "image_stretcher.hpp"
#include "d3d11_helpers.hpp"

#include <array>

#include <DirectXTex.h>
#include <gsl/gsl>

namespace sp::core {

Image_stretcher::Image_stretcher(ID3D11Device1& device,
                                 const Shader_database& shader_database) noexcept
   : _vs{std::get<0>(
        shader_database.groups.at("stretch_texture"s).vertex.at("main_vs"s).copy())},
     _ps{shader_database.groups.at("stretch_texture"s).pixel.at("main_ps"s).copy()},
     _ps_ms{
        shader_database.groups.at("stretch_texture"s).pixel.at("main_ms_ps"s).copy()},
     _constant_buffer{create_dynamic_constant_buffer(device, sizeof(Input_vars))}
{
}

void Image_stretcher::stretch(ID3D11DeviceContext1& dc, const D3D11_BOX& source_box,
                              const Game_rendertarget& source, const D3D11_BOX& dest_box,
                              const Game_rendertarget& dest) const noexcept
{
   Expects(source_box.front == 0 && source_box.back == 1);
   Expects(dest_box.front == 0 && dest_box.back == 1);

   // Update constant buffer
   Input_vars vars;

   vars.src_start = {source_box.left, source_box.top};
   vars.src_end = {source_box.right, source_box.bottom};

   update_dynamic_buffer(dc, *_constant_buffer, vars);

   // Setup state
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   dc.VSSetShader(_vs.get(), nullptr, 0);

   const CD3D11_VIEWPORT viewport{static_cast<float>(dest_box.left),
                                  static_cast<float>(dest_box.top),
                                  static_cast<float>(dest_box.right - dest_box.left),
                                  static_cast<float>(dest_box.bottom - dest_box.top)};
   auto* const cb = _constant_buffer.get();
   auto* const srv = source.srv.get();
   auto* const rtv = dest.rtv.get();

   dc.VSSetConstantBuffers(0, 1, &cb);

   dc.RSSetViewports(1, &viewport);

   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetShader(get_pixel_shader(source), nullptr, 0);

   // Draw
   dc.Draw(3, 0);
}

auto Image_stretcher::get_pixel_shader(const Game_rendertarget& source) const
   noexcept -> ID3D11PixelShader*
{
   return (source.sample_count == 1) ? _ps.get() : _ps_ms.get();
}

}
