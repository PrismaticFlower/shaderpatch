#pragma once

#include "com_ptr.hpp"

#include <array>

#include <glm/glm.hpp>

#include <d3d11_1.h>

namespace sp::effects {

inline auto srv_format(ID3D11ShaderResourceView& srv) noexcept -> DXGI_FORMAT
{
   D3D11_SHADER_RESOURCE_VIEW_DESC desc;
   srv.GetDesc(&desc);

   return desc.Format;
}

inline auto rtv_format(ID3D11RenderTargetView& rtv) noexcept -> DXGI_FORMAT
{
   D3D11_RENDER_TARGET_VIEW_DESC desc;
   rtv.GetDesc(&desc);

   return desc.Format;
}

inline void set_viewport(ID3D11DeviceContext1& dc, const UINT width,
                         const UINT height) noexcept
{
   const D3D11_VIEWPORT viewport{0,
                                 0,
                                 static_cast<float>(width),
                                 static_cast<float>(height),
                                 0.0f,
                                 1.0f};

   dc.RSSetViewports(1, &viewport);
}

inline auto create_additive_blend_state(ID3D11Device1& device) noexcept
   -> Com_ptr<ID3D11BlendState>
{
   CD3D11_BLEND_DESC desc{CD3D11_DEFAULT{}};

   desc.RenderTarget[0].BlendEnable = true;
   desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
   desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
   desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
   desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
   desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
   desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
   desc.RenderTarget[0].RenderTargetWriteMask = 0b1111;

   Com_ptr<ID3D11BlendState> blend_state;
   device.CreateBlendState(&desc, blend_state.clear_and_assign());

   return blend_state;
}

inline auto create_alpha_blend_state(ID3D11Device1& device) noexcept
   -> Com_ptr<ID3D11BlendState>
{
   CD3D11_BLEND_DESC desc{CD3D11_DEFAULT{}};

   desc.RenderTarget[0].BlendEnable = true;
   desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;
   desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
   desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
   desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
   desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
   desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
   desc.RenderTarget[0].RenderTargetWriteMask = 0b1111;

   Com_ptr<ID3D11BlendState> blend_state;
   device.CreateBlendState(&desc, blend_state.clear_and_assign());

   return blend_state;
}

}
