#pragma once

#include "../effects/postprocess.hpp"

#include "com_ptr.hpp"

#include <DirectXTex.h>
#include <d3d11_1.h>

namespace sp::core {

struct Effects_backbuffer {
   Effects_backbuffer() = default;

   Effects_backbuffer(ID3D11Device1& device, const DXGI_FORMAT format,
                      const UINT width, const UINT height) noexcept;

   auto game_rendertarget() const noexcept -> Game_rendertarget;

   auto postprocess_input() const noexcept -> effects::Postprocess_input;

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11RenderTargetView> game_rtv;
   Com_ptr<ID3D11ShaderResourceView> game_srv;
   Com_ptr<ID3D11ShaderResourceView> srv;
   DXGI_FORMAT format{};
   UINT width{};
   UINT height{};
};

}
