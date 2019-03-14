#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Depthstencil {
   Depthstencil(ID3D11Device1& device, const UINT width, const UINT height,
                const UINT sample_count) noexcept
   {
      const auto desc =
         CD3D11_TEXTURE2D_DESC{DXGI_FORMAT_D24_UNORM_S8_UINT,
                               width,
                               height,
                               1,
                               1,
                               D3D11_BIND_DEPTH_STENCIL,
                               D3D11_USAGE_DEFAULT,
                               0,
                               sample_count,
                               (sample_count != 1)
                                  ? static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN)
                                  : 0};

      device.CreateTexture2D(&desc, nullptr, texture.clear_and_assign());
      device.CreateDepthStencilView(texture.get(), nullptr, dsv.clear_and_assign());
   }

   Depthstencil() = default;
   Depthstencil(const Depthstencil&) = default;
   Depthstencil& operator=(const Depthstencil&) = default;
   Depthstencil(Depthstencil&&) = default;
   Depthstencil& operator=(Depthstencil&&) = default;
   ~Depthstencil() = default;

   explicit operator bool() noexcept
   {
      return texture != nullptr;
   }

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11DepthStencilView> dsv;
};

}
