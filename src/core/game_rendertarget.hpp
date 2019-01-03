#pragma once

#include <cstddef>

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Game_rendertarget {
   Game_rendertarget() = default;

   Game_rendertarget(ID3D11Device1& device, const DXGI_FORMAT format,
                     const UINT width, const UINT height) noexcept
      : width{static_cast<std::uint16_t>(width)}, height{static_cast<std::uint16_t>(height)}
   {
      const auto texture_desc =
         CD3D11_TEXTURE2D_DESC{format,
                               width,
                               height,
                               1,
                               1,
                               D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET |
                                  D3D11_BIND_UNORDERED_ACCESS};

      device.CreateTexture2D(&texture_desc, nullptr, texture.clear_and_assign());
      device.CreateRenderTargetView(texture.get(), nullptr, rtv.clear_and_assign());
      device.CreateShaderResourceView(texture.get(), nullptr, srv.clear_and_assign());
      device.CreateUnorderedAccessView(texture.get(), nullptr, uav.clear_and_assign());
   }

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11RenderTargetView> rtv;
   Com_ptr<ID3D11ShaderResourceView> srv;
   Com_ptr<ID3D11UnorderedAccessView> uav;
   std::uint16_t width;
   std::uint16_t height;

   bool alive() const noexcept
   {
      return texture != nullptr;
   }
};

}
