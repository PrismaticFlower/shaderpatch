#pragma once

#include "../effects/postprocess.hpp"
#include "com_ptr.hpp"
#include "game_rendertarget.hpp"

#include <d3d11_1.h>

namespace sp::core {

class Swapchain {
public:
   constexpr static auto format = DXGI_FORMAT_B8G8R8A8_UNORM;

   Swapchain(Com_ptr<ID3D11Device1> device, IDXGIAdapter2& adapter,
             const HWND window, const UINT width, const UINT height) noexcept;

   ~Swapchain() = default;

   Swapchain(const Swapchain&) = delete;
   Swapchain& operator=(const Swapchain&) = delete;

   Swapchain(Swapchain&&) = delete;
   Swapchain& operator=(Swapchain&&) = delete;

   void resize(const UINT width, const UINT height) noexcept;

   void present() noexcept;

   auto game_rendertarget() const noexcept -> Game_rendertarget;

   auto postprocess_output() const noexcept -> effects::Postprocess_output;

   auto width() const noexcept -> UINT;

   auto height() const noexcept -> UINT;

   auto texture() const noexcept -> ID3D11Texture2D*;

   auto rtv() const noexcept -> ID3D11RenderTargetView*;

   auto srv() const noexcept -> ID3D11ShaderResourceView*;

private:
   const Com_ptr<ID3D11Device1> _device;
   const Com_ptr<IDXGISwapChain1> _swapchain;
   Com_ptr<ID3D11Texture2D> _texture = [this] {
      Com_ptr<ID3D11Texture2D> texture;
      _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                            texture.void_clear_and_assign());

      return texture;
   }();
   Com_ptr<ID3D11RenderTargetView> _rtv = [this] {
      Com_ptr<ID3D11RenderTargetView> rtv;
      _device->CreateRenderTargetView(_texture.get(), nullptr, rtv.clear_and_assign());

      return rtv;
   }();
   Com_ptr<ID3D11ShaderResourceView> _srv = [this] {
      Com_ptr<ID3D11ShaderResourceView> srv;
      _device->CreateShaderResourceView(_texture.get(), nullptr,
                                        srv.clear_and_assign());

      return srv;
   }();

   const bool _allow_tearing;

   UINT _width;
   UINT _height;
};

}
