
#include "swapchain.hpp"
#include "../user_config.hpp"

#include <VersionHelpers.h>
#include <comdef.h>
#include <dxgi1_6.h>

namespace sp::core {

namespace {

constexpr auto swap_chain_buffers = 2;

auto get_swap_chain_effect() -> DXGI_SWAP_EFFECT
{
   if (IsWindows10OrGreater()) {
      return DXGI_SWAP_EFFECT_FLIP_DISCARD;
   }
   else if (IsWindows8OrGreater()) {
      return DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
   }

   return DXGI_SWAP_EFFECT_DISCARD;
}

auto create_swapchain(ID3D11Device1& device, IDXGIAdapter2& adapter,
                      const HWND window, const UINT width, const UINT height,
                      const bool allow_tearing) noexcept -> Com_ptr<IDXGISwapChain1>
{
   Com_ptr<IDXGIFactory2> factory;
   adapter.GetParent(__uuidof(IDXGIFactory2), factory.void_clear_and_assign());

   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;

   swap_chain_desc.Width = width;
   swap_chain_desc.Height = height;
   swap_chain_desc.Format = Swapchain::format;
   swap_chain_desc.Stereo = false;
   swap_chain_desc.SampleDesc = {1, 0};
   swap_chain_desc.BufferUsage =
      DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swap_chain_desc.BufferCount = swap_chain_buffers;
   swap_chain_desc.Scaling = DXGI_SCALING_NONE;
   swap_chain_desc.SwapEffect = get_swap_chain_effect();
   swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
   swap_chain_desc.Flags = allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

   Com_ptr<IDXGISwapChain1> swapchain;

   if (const auto result =
          factory->CreateSwapChainForHwnd(&device, window, &swap_chain_desc, nullptr,
                                          nullptr, swapchain.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create Direct3D 11 device! Reason: ",
                        _com_error{result}.ErrorMessage());
   }

   Com_ptr<IDXGIFactory1> parent;

   if (const auto result = swapchain->GetParent(__uuidof(IDXGIFactory1),
                                                parent.void_clear_and_assign());
       SUCCEEDED(result))

   {
      parent->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
   }

   return swapchain;
}

bool supports_tearing() noexcept
{
   Com_ptr<IDXGIFactory1> factory1;

   if (auto result =
          CreateDXGIFactory1(IID_IDXGIFactory1, factory1.void_clear_and_assign());
       SUCCEEDED(result)) {
      Com_ptr<IDXGIFactory5> factory5;
      if (result = factory1->QueryInterface(factory5.clear_and_assign());
          SUCCEEDED(result)) {
         BOOL supported;
         if (result = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                    &supported, sizeof(supported));
             SUCCEEDED(result)) {
            return supported != false;
         }
      }
   }

   return false;
}

}

Swapchain::Swapchain(Com_ptr<ID3D11Device1> device, IDXGIAdapter2& adapter,
                     const HWND window, const UINT width, const UINT height) noexcept
   : _device{device},
     _swapchain{create_swapchain(*device, adapter, window, width, height,
                                 supports_tearing() && user_config.window.allow_tearing)},
     _allow_tearing{supports_tearing() && user_config.window.allow_tearing},
     _width{width},
     _height{height}
{
}

void Swapchain::resize(const UINT width, const UINT height) noexcept
{
   _width = width;
   _height = height;

   _texture = nullptr;
   _rtv = nullptr;
   _srv = nullptr;

   _swapchain->ResizeBuffers(swap_chain_buffers, _width, _height, format,
                             _allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
   _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                         _texture.void_clear_and_assign());
   _device->CreateRenderTargetView(_texture.get(), nullptr, _rtv.clear_and_assign());
   _device->CreateShaderResourceView(_texture.get(), nullptr, _srv.clear_and_assign());
}

void Swapchain::present() noexcept
{
   const auto flags = _allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

   if (const auto result = _swapchain->Present(0, flags); FAILED(result)) {
      log_and_terminate("Frame Present call failed! reason: ",
                        _com_error{result}.ErrorMessage());
   }
}

auto Swapchain::game_rendertarget() const noexcept -> Game_rendertarget
{
   return {_texture, _rtv, _srv, nullptr, format, _width, _height};
}

auto Swapchain::postprocess_output() const noexcept -> effects::Postprocess_output
{
   return {*_rtv, format, _width, _height};
}

auto Swapchain::width() const noexcept -> UINT
{
   return _width;
}

auto Swapchain::height() const noexcept -> UINT
{
   return _height;
}

}
