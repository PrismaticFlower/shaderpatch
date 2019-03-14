#pragma once

#include "com_ptr.hpp"
#include "enum_flags.hpp"

#include <cstddef>

#include <d3d11_1.h>

namespace sp::core {
enum class Game_rt_flags : std::uint16_t {
   none = 0b0,
   presentation = 0b1,
   shadow = 0b10,
   flares = 0b100
};

constexpr bool marked_as_enum_flag(Game_rt_flags) noexcept
{
   return true;
}

struct Game_rendertarget {
   Game_rendertarget() = default;

   Game_rendertarget(ID3D11Device1& device, const DXGI_FORMAT format, const UINT width,
                     const UINT height, const UINT sample_count = 1,
                     Game_rt_flags flags = Game_rt_flags::none) noexcept;

   Game_rendertarget(Com_ptr<ID3D11Texture2D> texture,
                     Com_ptr<ID3D11RenderTargetView> rtv,
                     Com_ptr<ID3D11ShaderResourceView> srv, const DXGI_FORMAT format,
                     const UINT width, const UINT height, const UINT sample_count,
                     Game_rt_flags flags = Game_rt_flags::none) noexcept;

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11RenderTargetView> rtv;
   Com_ptr<ID3D11ShaderResourceView> srv;
   DXGI_FORMAT format;
   std::uint16_t width;
   std::uint16_t height;
   std::uint16_t sample_count;
   Game_rt_flags flags;

   bool alive() const noexcept
   {
      return texture != nullptr;
   }
};
}
