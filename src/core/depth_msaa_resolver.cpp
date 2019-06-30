
#include "depth_msaa_resolver.hpp"
#include "../logger.hpp"

namespace sp::core {

Depth_msaa_resolver::Depth_msaa_resolver(ID3D11Device5& device,
                                         const Shader_database& shader_database) noexcept
   : _vs{std::get<0>(
        shader_database.groups.at("depth msaa resolve"s).vertex.at("main_vs"s).copy())},
     _ps_x2{
        shader_database.groups.at("depth msaa resolve"s).pixel.at("main_x2_ps"s).copy()},
     _ps_x4{
        shader_database.groups.at("depth msaa resolve"s).pixel.at("main_x4_ps"s).copy()},
     _ps_x8{
        shader_database.groups.at("depth msaa resolve"s).pixel.at("main_x8_ps"s).copy()},
     _ds_state{[&] {
        CD3D11_DEPTH_STENCIL_DESC desc{CD3D11_DEFAULT{}};

        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;

        Com_ptr<ID3D11DepthStencilState> state;

        device.CreateDepthStencilState(&desc, state.clear_and_assign());

        return state;
     }()}
{
}

void Depth_msaa_resolver::resolve(ID3D11DeviceContext4& dc, const Depthstencil& source,
                                  const Depthstencil& dest) const noexcept
{
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_vs.get(), nullptr, 0);

   D3D11_TEXTURE2D_DESC src_desc;
   source.texture->GetDesc(&src_desc);

   const CD3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(src_desc.Width),
                                  static_cast<float>(src_desc.Width)};
   dc.RSSetViewports(1, &viewport);

   dc.OMSetDepthStencilState(_ds_state.get(), 0x0);
   dc.OMSetRenderTargets(0, nullptr, dest.dsv.get());

   auto* const srv = source.srv.get();
   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetShader(get_pixel_shader(src_desc.SampleDesc.Count), nullptr, 0);

   dc.Draw(3, 0);
}

auto Depth_msaa_resolver::get_pixel_shader(const UINT sample_count) const
   noexcept -> ID3D11PixelShader*
{
   if (sample_count == 2) return _ps_x2.get();
   if (sample_count == 4) return _ps_x4.get();
   if (sample_count == 8) return _ps_x8.get();

   log_and_terminate("Attempt to resolve an MSAA depth surface with an "
                     "unexpected sample count!");
}

}
