
#include "swapchain.hpp"
#include "../user_config.hpp"

#include <comdef.h>
#include <dxgi1_6.h>

namespace sp::core {

namespace {

constexpr auto swap_chain_buffers = 2;

auto create_swapchain(ID3D11Device1& device, const HWND window, const UINT width,
                      const UINT height, const UINT flags) noexcept
   -> Com_ptr<IDXGISwapChain1>
{
   Com_ptr<IDXGIDevice> dxgi_device;

   if (FAILED(device.QueryInterface(dxgi_device.clear_and_assign()))) {
      log_and_terminate(
         "Failed to create swap chain. Unable to get DXGI device.");
   }

   Com_ptr<IDXGIAdapter1> adapter;

   if (FAILED(dxgi_device->GetParent(IID_PPV_ARGS(adapter.clear_and_assign())))) {
      log_and_terminate(
         "Failed to create swap chain. Unable to get DXGI adapter.");
   }

   Com_ptr<IDXGIFactory2> factory;

   if (FAILED(adapter->GetParent(IID_PPV_ARGS(factory.clear_and_assign())))) {
      log_and_terminate(
         "Failed to create swap chain. Unable to get DXGI factory.");
   }

   DXGI_SWAP_CHAIN_DESC1 swap_chain_desc;

   swap_chain_desc.Width = width;
   swap_chain_desc.Height = height;
   swap_chain_desc.Format = Swapchain::format;
   swap_chain_desc.Stereo = false;
   swap_chain_desc.SampleDesc = {1, 0};
   swap_chain_desc.BufferUsage =
      DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swap_chain_desc.BufferCount = swap_chain_buffers;
   swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
   swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
   swap_chain_desc.Flags = flags;

   Com_ptr<IDXGISwapChain1> swapchain;

   if (const auto result =
          factory->CreateSwapChainForHwnd(&device, window, &swap_chain_desc, nullptr,
                                          nullptr, swapchain.clear_and_assign());
       FAILED(result)) {
      log_and_terminate_fmt("Failed to create DXGI swapchain! Reason: {}",
                            _com_error{result}.ErrorMessage());
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

Swapchain::Swapchain(Com_ptr<ID3D11Device1> device, const HWND window,
                     const UINT width, const UINT height) noexcept
   : _window{window},
     _supports_tearing{supports_tearing()},
     _fullscreen{false},
     _width{width},
     _height{height},
     _flags{DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH |
            (_supports_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u)},
     _device{device},
     _swapchain{create_swapchain(*device, _window, _width, _height, _flags)}
{
   _swapchain->GetBuffer(0, IID_PPV_ARGS(_texture.clear_and_assign()));
   _device->CreateRenderTargetView(_texture.get(), nullptr, _rtv.clear_and_assign());
   _device->CreateShaderResourceView(_texture.get(), nullptr, _srv.clear_and_assign());
}

void Swapchain::reset(ID3D11DeviceContext& dc) noexcept
{
   _texture = nullptr;
   _rtv = nullptr;
   _srv = nullptr;

   dc.ClearState();
   dc.Flush();

   resize(_fullscreen, _width, _height);
}

void Swapchain::resize(const bool fullscreen, const UINT width, const UINT height) noexcept
{
   _width = width;
   _height = height;
   _fullscreen = fullscreen;

   _texture = nullptr;
   _rtv = nullptr;
   _srv = nullptr;

   _swapchain->SetFullscreenState(_fullscreen, nullptr);
   _swapchain->ResizeBuffers(swap_chain_buffers, _width, _height, format, _flags);

   _swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                         _texture.void_clear_and_assign());
   _device->CreateRenderTargetView(_texture.get(), nullptr, _rtv.clear_and_assign());
   _device->CreateShaderResourceView(_texture.get(), nullptr, _srv.clear_and_assign());
}

auto Swapchain::present() noexcept -> Present_status
{
   BOOL fullscreen_state = false;
   _swapchain->GetFullscreenState(&fullscreen_state, nullptr);

   if ((fullscreen_state != 0) != _fullscreen) {
      _fullscreen = not _fullscreen;

      return Present_status::needs_reset;
   }

   const bool allow_tearing = not user_config.display.v_sync and _supports_tearing;
   const UINT sync_interval = allow_tearing ? 0 : 1;
   const UINT flags = !_fullscreen && allow_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

   if (const HRESULT result = _swapchain->Present(sync_interval, flags);
       FAILED(result)) {
      log_and_terminate(Log_level::error, "Frame Present call failed! reason: ",
                        _com_error{result}.ErrorMessage());
   }

   return Present_status::ok;
}

auto Swapchain::game_rendertarget() const noexcept -> Game_rendertarget
{
   return {_texture, _rtv,    _srv, format,
           _width,   _height, 1,    Game_rt_type::presentation};
}

auto Swapchain::postprocess_output() const noexcept -> effects::Postprocess_output
{
   return {*_rtv, _width, _height};
}

auto Swapchain::width() const noexcept -> UINT
{
   return _width;
}

auto Swapchain::height() const noexcept -> UINT
{
   return _height;
}

auto Swapchain::texture() const noexcept -> ID3D11Texture2D*
{
   return _texture.get();
}

auto Swapchain::rtv() const noexcept -> ID3D11RenderTargetView*
{
   return _rtv.get();
}

auto Swapchain::srv() const noexcept -> ID3D11ShaderResourceView*
{
   return _srv.get();
}

}
