
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

   const glm::uvec2 dest_start{dest_box.left, dest_box.top};
   const glm::uvec2 dest_end = {dest_box.right, dest_box.bottom};
   vars.dest_length = dest_end - dest_start;

   vars.src_start = {source_box.left, source_box.top};
   vars.src_length = glm::uvec2{source_box.right, source_box.bottom} - vars.src_start;

   update_dynamic_buffer(dc, *_constant_buffer, vars);

   // Setup state
   dc.IASetInputLayout(nullptr);
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   dc.VSSetShader(_vs.get(), nullptr, 0);
   dc.HSSetShader(nullptr, nullptr, 0);
   dc.DSSetShader(nullptr, nullptr, 0);
   dc.GSSetShader(nullptr, nullptr, 0);

   const CD3D11_VIEWPORT viewport{static_cast<float>(dest_box.left),
                                  static_cast<float>(dest_box.top),
                                  static_cast<float>(dest_box.right),
                                  static_cast<float>(dest_box.bottom)};
   auto* const cb = _constant_buffer.get();
   auto* const srv = source.srv.get();
   auto* const rtv = dest.rtv.get();

   dc.RSSetState(nullptr);
   dc.RSSetViewports(1, &viewport);

   dc.OMSetDepthStencilState(nullptr, 0xffu);
   dc.OMSetBlendState(nullptr, nullptr, 0xffffffffu);
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetConstantBuffers(0, 1, &cb);
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
