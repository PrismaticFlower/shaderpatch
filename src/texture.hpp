#pragma once

#include "com_ptr.hpp"
#include "patch_texture.hpp"

#include <utility>

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <d3d9.h>

namespace sp {

class Texture {
public:
   Texture() = default;

   Texture(Com_ptr<IDirect3DDevice9> device,
           Com_ptr<IDirect3DBaseTexture9> d3d_texture, Sampler_info sampler_info)
      : _device{std::move(device)}, _texture{std::move(d3d_texture)}, _sampler{sampler_info}
   {
      switch (_texture->GetType()) {
      case D3DRTYPE_TEXTURE: {

         D3DSURFACE_DESC desc{};
         static_cast<IDirect3DTexture9&>(*_texture).GetLevelDesc(0, &desc);

         _size = {desc.Width, desc.Height};

         break;
      }
      case D3DRTYPE_VOLUMETEXTURE: {
         D3DVOLUME_DESC desc{};
         static_cast<IDirect3DVolumeTexture9&>(*_texture).GetLevelDesc(0, &desc);

         _size = {desc.Width, desc.Height};

         break;
      }
      case D3DRTYPE_CUBETEXTURE: {
         D3DSURFACE_DESC desc{};
         static_cast<IDirect3DCubeTexture9&>(*_texture).GetLevelDesc(0, &desc);

         _size = {desc.Width, desc.Height};

         break;
      }
      }
   };

   void bind(DWORD slot) const noexcept
   {
      if (empty()) return;

      _device->SetSamplerState(slot, D3DSAMP_ADDRESSU, _sampler.address_mode_u);
      _device->SetSamplerState(slot, D3DSAMP_ADDRESSV, _sampler.address_mode_v);
      _device->SetSamplerState(slot, D3DSAMP_ADDRESSW, _sampler.address_mode_w);
      _device->SetSamplerState(slot, D3DSAMP_MAGFILTER, _sampler.mag_filter);
      _device->SetSamplerState(slot, D3DSAMP_MINFILTER, _sampler.min_filter);
      _device->SetSamplerState(slot, D3DSAMP_MIPFILTER, _sampler.mip_filter);
      _device->SetSamplerState(slot, D3DSAMP_MIPMAPLODBIAS,
                               reinterpret_cast<const DWORD&>(_sampler.mip_lod_bias));
      _device->SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, _sampler.srgb);

      _device->SetTexture(slot, _texture.get());
   }

   bool empty() const noexcept
   {
      return (!_device || !_texture);
   }

   glm::ivec2 size() const noexcept
   {
      return _size;
   }

   explicit operator bool() const noexcept
   {
      return !empty();
   }

private:
   Com_ptr<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DBaseTexture9> _texture;
   glm::ivec2 _size{};
   Sampler_info _sampler{};
};

}
