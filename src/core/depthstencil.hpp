#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Depthstencil {
   Depthstencil(ID3D11Device1& device, const UINT width, const UINT height,
                const UINT sample_count) noexcept
   {
      {
         const auto desc =
            CD3D11_TEXTURE2D_DESC{DXGI_FORMAT_R24G8_TYPELESS,
                                  width,
                                  height,
                                  1,
                                  1,
                                  D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
                                  D3D11_USAGE_DEFAULT,
                                  0,
                                  sample_count,
                                  (sample_count != 1)
                                     ? static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN)
                                     : 0};

         device.CreateTexture2D(&desc, nullptr, texture.clear_and_assign());
      }

      const bool ms = sample_count != 1;

      {
         const CD3D11_DEPTH_STENCIL_VIEW_DESC desc{ms ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                      : D3D11_DSV_DIMENSION_TEXTURE2D,
                                                   DXGI_FORMAT_D24_UNORM_S8_UINT};

         device.CreateDepthStencilView(texture.get(), &desc, dsv.clear_and_assign());
      }

      {
         const CD3D11_DEPTH_STENCIL_VIEW_DESC desc{ms ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                      : D3D11_DSV_DIMENSION_TEXTURE2D,
                                                   DXGI_FORMAT_D24_UNORM_S8_UINT,
                                                   0,
                                                   0,
                                                   1,
                                                   D3D11_DSV_READ_ONLY_DEPTH |
                                                      D3D11_DSV_READ_ONLY_STENCIL};

         device.CreateDepthStencilView(texture.get(), &desc,
                                       dsv_readonly.clear_and_assign());
      }

      {
         const CD3D11_SHADER_RESOURCE_VIEW_DESC desc{ms ? D3D11_SRV_DIMENSION_TEXTURE2DMS
                                                        : D3D11_SRV_DIMENSION_TEXTURE2D,
                                                     DXGI_FORMAT_R24_UNORM_X8_TYPELESS};

         device.CreateShaderResourceView(texture.get(), &desc, srv.clear_and_assign());
      }
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
   Com_ptr<ID3D11DepthStencilView> dsv_readonly;
   Com_ptr<ID3D11ShaderResourceView> srv;

   constexpr static auto default_format = DXGI_FORMAT_R32G8X24_TYPELESS;
};

}
