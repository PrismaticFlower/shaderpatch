#pragma once

#include <cstddef>

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Game_rendertarget {
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
