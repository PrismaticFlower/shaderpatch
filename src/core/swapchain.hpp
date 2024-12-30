#pragma once

#include "../effects/postprocess.hpp"
#include "com_ptr.hpp"
#include "game_rendertarget.hpp"

#include <d3d11_1.h>

namespace sp::core {

enum class Present_status { ok, needs_reset };

class Swapchain {
public:
   constexpr static auto format = DXGI_FORMAT_R8G8B8A8_UNORM;

   Swapchain(Com_ptr<ID3D11Device1> device, const HWND window, const UINT width,
             const UINT height) noexcept;

   ~Swapchain() = default;

   Swapchain(const Swapchain&) = delete;
   Swapchain& operator=(const Swapchain&) = delete;

   Swapchain(Swapchain&&) = delete;
   Swapchain& operator=(Swapchain&&) = delete;

   void reset(ID3D11DeviceContext& dc) noexcept;

   void resize(const bool fullscreen, const UINT width, const UINT height) noexcept;

   auto present() noexcept -> Present_status;

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
   Com_ptr<ID3D11Texture2D> _texture;
   Com_ptr<ID3D11RenderTargetView> _rtv;
   Com_ptr<ID3D11ShaderResourceView> _srv;

   const HWND _window;

   const bool _allow_tearing;

   bool _fullscreen = false;
   UINT _width = 0;
   UINT _height = 0;
};

}
