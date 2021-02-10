#pragma once

#include "com_ptr.hpp"

#include <cstddef>

#include <d3d11_1.h>

namespace sp::core {
enum class Game_rt_type : std::uint16_t {
   untyped,
   presentation,
   farscene,
   farscene_shadow,
   shadow,
   flares
};

struct Game_rendertarget {
   Game_rendertarget() = default;

   Game_rendertarget(ID3D11Device1& device, const DXGI_FORMAT format, const UINT width,
                     const UINT height, const UINT sample_count = 1,
                     const Game_rt_type type = Game_rt_type::untyped) noexcept;

   Game_rendertarget(Com_ptr<ID3D11Texture2D> texture,
                     Com_ptr<ID3D11RenderTargetView> rtv,
                     Com_ptr<ID3D11ShaderResourceView> srv, const DXGI_FORMAT format,
                     const UINT width, const UINT height, const UINT sample_count,
                     const Game_rt_type type = Game_rt_type::untyped) noexcept;

   Game_rendertarget(ID3D11Device1& device, const DXGI_FORMAT resource_format,
                     const DXGI_FORMAT view_format, const UINT width,
                     const UINT height, const D3D11_BIND_FLAG extra_bind_flags,
                     const Game_rt_type type = Game_rt_type::untyped) noexcept;

   Com_ptr<ID3D11Texture2D> texture;
   Com_ptr<ID3D11RenderTargetView> rtv;
   Com_ptr<ID3D11ShaderResourceView> srv;
   DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
   std::uint16_t width{};
   std::uint16_t height{};
   std::uint16_t sample_count{};
   Game_rt_type type = Game_rt_type::untyped;

   explicit operator bool() const noexcept
   {
      return texture != nullptr;
   }
};
}
