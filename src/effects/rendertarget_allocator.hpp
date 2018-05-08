#pragma once

#include "com_ptr.hpp"
#include "com_ref.hpp"
#include "throw_if_failed.hpp"

#include <memory>
#include <tuple>

#include <boost/container/flat_map.hpp>
#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <boost/smart_ptr/make_local_shared.hpp>
#include <glm/vec2.hpp>
#include <gsl/gsl>

#include <d3d9.h>

namespace sp::effects {

class Rendertarget_allocator {
private:
   template<typename Type>
   using Local_ptr = boost::local_shared_ptr<Type>;
   template<typename Key, typename Value>
   using Container = boost::container::flat_multimap<Key, Value>;

public:
   Rendertarget_allocator(Com_ref<IDirect3DDevice9> device) : _device{device}
   {
      _cache = {boost::make_local_shared<
         Container<std::tuple<D3DFORMAT, int, int>, Com_ptr<IDirect3DTexture9>>>()};
   }

   class Handle {
   public:
      Handle(const Handle&) = delete;
      Handle& operator=(const Handle&) = delete;
      Handle(Handle&&) = default;
      Handle& operator=(Handle&&) = delete;

      ~Handle()
      {
         D3DSURFACE_DESC desc;
         _rt_texture->GetLevelDesc(0, &desc);

         _cache->emplace(std::make_tuple(desc.Format, static_cast<int>(desc.Width),
                                         static_cast<int>(desc.Height)),
                         std::move(_rt_texture));
      };

      gsl::not_null<IDirect3DTexture9*> texture() const noexcept
      {
         return gsl::not_null<IDirect3DTexture9*>{_rt_texture.get()};
      }

      gsl::not_null<IDirect3DSurface9*> surface() const noexcept
      {
         return gsl::not_null<IDirect3DSurface9*>{_rt_surface.get()};
      }

   private:
      friend Rendertarget_allocator;

      Handle(gsl::not_null<Com_ptr<IDirect3DTexture9>> rt_texture,
             gsl::not_null<Local_ptr<Container<std::tuple<D3DFORMAT, int, int>, Com_ptr<IDirect3DTexture9>>>> cache)
         : _rt_texture{std::move(rt_texture)}, _cache{std::move(cache)}
      {
         _rt_texture->GetSurfaceLevel(0, _rt_surface.clear_and_assign());
      }

      Com_ptr<IDirect3DTexture9> _rt_texture;
      Com_ptr<IDirect3DSurface9> _rt_surface;

      Local_ptr<Container<std::tuple<D3DFORMAT, int, int>, Com_ptr<IDirect3DTexture9>>> _cache;
   };

   auto allocate(D3DFORMAT format, glm::ivec2 resolution) -> Handle
   {
      if (auto cached = _cache->find(std::tie(format, resolution.x, resolution.y));
          cached != std::end(*_cache)) {
         Com_ptr<IDirect3DTexture9> rt = std::move(cached->second);
         _cache->erase(cached);

         return {gsl::not_null<decltype(rt)>{std::move(rt)},
                 gsl::not_null<decltype(_cache)>{_cache}};
      }

      return create(format, resolution);
   }

   void reset() noexcept
   {
      _cache->clear();
   }

private:
   auto create(D3DFORMAT format, glm::ivec2 resolution) -> Handle
   {
      Com_ptr<IDirect3DTexture9> rt;

      throw_if_failed(_device->CreateTexture(resolution.x, resolution.y, 1,
                                             D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT,
                                             rt.clear_and_assign(), nullptr));

      return {gsl::not_null<decltype(rt)>{std::move(rt)},
              gsl::not_null<decltype(_cache)>{_cache}};
   }

   Com_ref<IDirect3DDevice9> _device;

   Local_ptr<Container<std::tuple<D3DFORMAT, int, int>, Com_ptr<IDirect3DTexture9>>> _cache;
};
}
