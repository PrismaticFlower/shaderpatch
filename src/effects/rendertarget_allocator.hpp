#pragma once

#include "../logger.hpp"
#include "com_ptr.hpp"
#include "enum_flags.hpp"

#include <tuple>
#include <vector>

#include <comdef.h>
#include <d3d11_1.h>

namespace sp::effects {

constexpr static UINT rendertarget_bind_srv_rtv =
   D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

constexpr static UINT rendertarget_bind_srv_rtv_uav =
   D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS;

struct Rendertarget_desc {
   DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
   UINT width = 0;
   UINT height = 0;
   UINT bind_flags = 0;
   DXGI_FORMAT srv_format = format;
   DXGI_FORMAT rtv_format = format;
   DXGI_FORMAT uav_format = format;

   bool operator==(const Rendertarget_desc&) const noexcept = default;
};

class Rendertarget_allocator {
private:
   struct Rendertarget {
      Rendertarget_desc desc;

      Com_ptr<ID3D11ShaderResourceView> srv;
      Com_ptr<ID3D11RenderTargetView> rtv;
      Com_ptr<ID3D11UnorderedAccessView> uav;
      Com_ptr<ID3D11Texture2D> texture;

      explicit operator bool() const noexcept
      {
         return texture != nullptr;
      }
   };

public:
   Rendertarget_allocator(Com_ptr<ID3D11Device1> device) : _device{device} {}

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
         if (_rendertarget) {
            _allocator.return_rendertarget(std::move(_rendertarget));
         }
      }

      auto srv() const noexcept -> ID3D11ShaderResourceView&
      {
         return *_rendertarget.srv;
      }

      auto rtv() const noexcept -> ID3D11RenderTargetView&
      {
         return *_rendertarget.rtv;
      }

      auto uav() const noexcept -> ID3D11UnorderedAccessView&
      {
         return *_rendertarget.uav;
      }

      auto texture() const noexcept -> ID3D11Texture2D&
      {
         return *_rendertarget.texture;
      }

   private:
      Rendertarget _rendertarget;
      Rendertarget_allocator& _allocator;
   };

   auto allocate(const Rendertarget_desc& desc) noexcept -> Handle
   {
      if (auto cached = find(desc); cached != std::end(_rendertargets)) {

         auto rendertarget = std::move(*cached);
         _rendertargets.erase(cached);

         return {*this, rendertarget};
      }

      return create(desc);
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
   auto find(const Rendertarget_desc& desc) noexcept -> std::vector<Rendertarget>::iterator
   {
      return std::find_if(_rendertargets.begin(), _rendertargets.end(),
                          [&](const Rendertarget& target) noexcept {
                             return target.desc == desc;
                          });
   }

   auto create(const Rendertarget_desc& desc) noexcept -> Handle
   {
      Rendertarget rendertarget;

      const auto texture_desc =
         CD3D11_TEXTURE2D_DESC{desc.format, desc.width, desc.height,
                               1,           1,          desc.bind_flags};

      if (const auto result =
             _device->CreateTexture2D(&texture_desc, nullptr,
                                      rendertarget.texture.clear_and_assign());
          FAILED(result)) {
         log_and_terminate("Failed to create rendertarget texture! reason: {}",
                           _com_error{result}.ErrorMessage());
      }

      if (desc.bind_flags & D3D11_BIND_SHADER_RESOURCE) {
         const auto srv_desc =
            CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D,
                                             desc.srv_format};

         if (const auto result =
                _device->CreateShaderResourceView(rendertarget.texture.get(), &srv_desc,
                                                  rendertarget.srv.clear_and_assign());
             FAILED(result)) {
            log_and_terminate("Failed to create rendertarget SRV! reason: {}",
                              _com_error{result}.ErrorMessage());
         }
      }

      if (desc.bind_flags & D3D11_BIND_RENDER_TARGET) {
         const auto rtv_desc =
            CD3D11_RENDER_TARGET_VIEW_DESC{D3D11_RTV_DIMENSION_TEXTURE2D,
                                           desc.rtv_format};

         if (const auto result =
                _device->CreateRenderTargetView(rendertarget.texture.get(), &rtv_desc,
                                                rendertarget.rtv.clear_and_assign());
             FAILED(result)) {
            log_and_terminate("Failed to create rendertarget RTV! reason: {}",
                              _com_error{result}.ErrorMessage());
         }
      }

      if (desc.bind_flags & D3D11_BIND_UNORDERED_ACCESS) {
         const auto uav_desc =
            CD3D11_UNORDERED_ACCESS_VIEW_DESC{D3D11_UAV_DIMENSION_TEXTURE2D,
                                              desc.uav_format};

         if (const auto result =
                _device->CreateUnorderedAccessView(rendertarget.texture.get(), &uav_desc,
                                                   rendertarget.uav.clear_and_assign());
             FAILED(result)) {
            log_and_terminate("Failed to create rendertarget UAV! reason: {}",
                              _com_error{result}.ErrorMessage());
         }
      }

      rendertarget.desc = desc;

      return {*this, std::move(rendertarget)};
   }

   const Com_ptr<ID3D11Device1> _device;

   std::vector<Rendertarget> _rendertargets;
};
}
