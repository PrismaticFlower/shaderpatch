#pragma once

#include "com_ptr.hpp"
#include "patch_texture.hpp"

#include <utility>

#include <d3d9.h>

namespace sp {

class Texture {
public:
   Texture(Com_ptr<IDirect3DDevice9> device,
           Com_ptr<IDirect3DBaseTexture9> d3d_texture, Sampler_info sampler_info)
      : _device{std::move(device)}, _texture{std::move(d3d_texture)}, _sampler{sampler_info} {};

   Texture() = delete;

   void bind(DWORD slot) const noexcept
   {
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

private:
   Com_ptr<IDirect3DDevice9> _device;
   Com_ptr<IDirect3DBaseTexture9> _texture;
   Sampler_info _sampler;
};
}
