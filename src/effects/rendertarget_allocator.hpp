#pragma once

#include "../logger.hpp"
#include "com_ptr.hpp"

#include <tuple>
#include <vector>

#include <comdef.h>
#include <d3d11_1.h>

namespace sp::effects {

class Rendertarget_allocator {
public:
   Rendertarget_allocator(Com_ptr<ID3D11Device1> device) : _device{device} {}

   struct Rendertarget {
      UINT width = 0;
      UINT height = 0;
      DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

      Com_ptr<ID3D11ShaderResourceView> srv;
      Com_ptr<ID3D11RenderTargetView> rtv;
   };

   class Handle {
   public:
      Handle(Rendertarget_allocator& allocator, Rendertarget rendertarget)
         : _allocator{allocator}, _rendertarget{std::move(rendertarget)}
      {
      }

      Handle(const Handle&) = delete;
      Handle& operator=(const Handle&) = delete;
      Handle(Handle&&) = default;
      Handle& operator=(Handle&&) = delete;

      ~Handle()
      {
         _allocator.return_rendertarget(std::move(_rendertarget));
      }

      auto srv() const noexcept -> ID3D11ShaderResourceView&
      {
         return *_rendertarget.srv;
      }

      auto rtv() const noexcept -> ID3D11RenderTargetView&
      {
         return *_rendertarget.rtv;
      }

      auto rendertarget() const noexcept -> const Rendertarget&
      {
         return _rendertarget;
      }

   private:
      Rendertarget _rendertarget;
      Rendertarget_allocator& _allocator;
   };

   auto allocate(const DXGI_FORMAT format, const UINT width, const UINT height) noexcept
      -> Handle
   {
      if (auto cached = find(format, width, height);
          cached != std::cend(_rendertargets)) {

         auto rendertarget = std::move(*cached);
         _rendertargets.erase(cached);

         return {*this, rendertarget};
      }

      return create(format, width, height);
   }

   void return_rendertarget(Rendertarget rendertarget) noexcept
   {
      _rendertargets.emplace_back(std::move(rendertarget));
   }

   void reset() noexcept
   {
      _rendertargets.clear();
   }

private:
   auto find(const DXGI_FORMAT format, const UINT width, const UINT height)
      -> std::vector<Rendertarget>::const_iterator
   {
      return std::find_if(
         _rendertargets.cbegin(),
         _rendertargets.cend(), [&](const Rendertarget& target) noexcept {
            return std::tie(width, height, format) ==
                   std::tie(target.width, target.height, target.format);
         });
   }

   auto create(const DXGI_FORMAT format, const UINT width, const UINT height) noexcept
      -> Handle
   {
      const auto texture_desc =
         CD3D11_TEXTURE2D_DESC{format,
                               width,
                               height,
                               1,
                               1,
                               D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET};

      Com_ptr<ID3D11Texture2D> texture;

      if (const auto result = _device->CreateTexture2D(&texture_desc, nullptr,
                                                       texture.clear_and_assign());
          FAILED(result)) {
         log_and_terminate("Failed to create rendertarget texture! reason: ",
                           _com_error{result}.ErrorMessage());
      }

      Rendertarget rendertarget;

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), nullptr,
                                               rendertarget.srv.clear_and_assign());
          FAILED(result)) {
         log_and_terminate("Failed to create rendertarget SRV! reason: ",
                           _com_error{result}.ErrorMessage());
      }

      if (const auto result =
             _device->CreateRenderTargetView(texture.get(), nullptr,
                                             rendertarget.rtv.clear_and_assign());
          FAILED(result)) {
         log_and_terminate("Failed to create rendertarget RTV! reason: ",
                           _com_error{result}.ErrorMessage());
      }

      rendertarget.width = width;
      rendertarget.height = height;
      rendertarget.format = format;

      return {*this, std::move(rendertarget)};
   }

   const Com_ptr<ID3D11Device1> _device;

   std::vector<Rendertarget> _rendertargets;
};
}
