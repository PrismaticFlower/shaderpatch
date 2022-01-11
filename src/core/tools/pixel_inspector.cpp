
#include "pixel_inspector.hpp"

#include "../../imgui/imgui.h"

namespace sp::core::tools {

namespace {

constexpr auto inspect_region_size = 32;
constexpr auto inspect_pixel_size = 8;

auto make_texture(ID3D11Device5& device) noexcept -> Com_ptr<ID3D11Texture2D>
{
   Com_ptr<ID3D11Texture2D> texture;

   D3D11_TEXTURE2D_DESC desc{.Width = inspect_region_size,
                             .Height = inspect_region_size,
                             .MipLevels = 1,
                             .ArraySize = 1,
                             .Format = Swapchain::format,
                             .SampleDesc = {1, 0},
                             .Usage = D3D11_USAGE_DEFAULT,
                             .BindFlags = D3D11_BIND_SHADER_RESOURCE};

   device.CreateTexture2D(&desc, nullptr, texture.clear_and_assign());

   return texture;
}

auto make_srv(ID3D11Device5& device, ID3D11Texture2D& texture) noexcept
   -> Com_ptr<ID3D11ShaderResourceView>
{
   Com_ptr<ID3D11ShaderResourceView> srv;

   device.CreateShaderResourceView(&texture, nullptr, srv.clear_and_assign());

   return srv;
}

}

Pixel_inspector::Pixel_inspector(Com_ptr<ID3D11Device5> device, shader::Database& shaders)
   : _vs{std::get<0>(shaders.vertex("postprocess"sv).entrypoint("main_vs"sv))},
     _ps{shaders.pixel("pixel inspector"sv).entrypoint("main_ps"sv)},
     _texture{make_texture(*device)},
     _srv{make_srv(*device, *_texture)}
{
}

void Pixel_inspector::show(ID3D11DeviceContext1& dc, Swapchain& swapchain,
                           const HWND window, const UINT swapchain_scale) noexcept
{
   if (!ImGui::GetIO().WantCaptureMouse && (GetKeyState(VK_LBUTTON) & 0x8000)) {
      if (!GetCursorPos(&_window_pos)) return;
      if (!ScreenToClient(window, &_window_pos)) return;
   }

   POINT pos = _window_pos;
   pos.x = pos.x * swapchain_scale / 100;
   pos.y = pos.y * swapchain_scale / 100;

   dc.ClearState();

   const D3D11_BOX copy_box{static_cast<UINT>(pos.x),
                            static_cast<UINT>(pos.y),
                            0,
                            static_cast<UINT>(pos.x) + inspect_region_size,
                            static_cast<UINT>(pos.y) + inspect_region_size,
                            1};

   const D3D11_VIEWPORT viewport{.TopLeftX = static_cast<float>(pos.x),
                                 .TopLeftY = static_cast<float>(pos.y),
                                 .Width = static_cast<float>(
                                    inspect_region_size * inspect_pixel_size),
                                 .Height = static_cast<float>(
                                    inspect_region_size * inspect_pixel_size),
                                 .MinDepth = 0.0f,
                                 .MaxDepth = 1.0f};

   auto* srv = _srv.get();
   auto* rtv = swapchain.rtv();

   dc.CopySubresourceRegion(_texture.get(), 0, 0, 0, 0, swapchain.texture(), 0,
                            &copy_box);

   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_vs.get(), nullptr, 0);
   dc.RSSetViewports(1, &viewport);
   dc.PSSetShader(_ps.get(), nullptr, 0);
   dc.PSSetShaderResources(0, 1, &srv);
   dc.OMSetRenderTargets(1, &rtv, nullptr);
   dc.Draw(3, 0);
}

}