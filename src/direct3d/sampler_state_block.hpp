#pragma once

#include <array>

#include <d3d9.h>

namespace sp::direct3d {

class Sampler_state_block {
public:
   template<typename Type = DWORD>
   Type& at(const D3DSAMPLERSTATETYPE state) noexcept
   {
      const auto index = static_cast<int>(state) - 1;

      return reinterpret_cast<Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   const Type& at(const D3DSAMPLERSTATETYPE state) const noexcept
   {
      const auto index = static_cast<int>(state) - 1;

      return reinterpret_cast<const Type&>(_state_block[index]);
   }

   template<typename Type = DWORD>
   Type get(const D3DSAMPLERSTATETYPE state) const noexcept
   {
      return at<Type>(state);
   }

   template<typename Type = DWORD>
   Type set(const D3DSAMPLERSTATETYPE state, const Type value) noexcept
   {
      return at<Type>(state) = value;
   }

   void apply(IDirect3DDevice9& device, int sampler) const noexcept
   {
      device.SetSamplerState(sampler, D3DSAMP_ADDRESSU, at(D3DSAMP_ADDRESSU));
      device.SetSamplerState(sampler, D3DSAMP_ADDRESSV, at(D3DSAMP_ADDRESSV));
      device.SetSamplerState(sampler, D3DSAMP_ADDRESSW, at(D3DSAMP_ADDRESSW));
      device.SetSamplerState(sampler, D3DSAMP_BORDERCOLOR, at(D3DSAMP_BORDERCOLOR));
      device.SetSamplerState(sampler, D3DSAMP_MAGFILTER, at(D3DSAMP_MAGFILTER));
      device.SetSamplerState(sampler, D3DSAMP_MINFILTER, at(D3DSAMP_MINFILTER));
      device.SetSamplerState(sampler, D3DSAMP_MIPFILTER, at(D3DSAMP_MIPFILTER));
      device.SetSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS, at(D3DSAMP_MIPMAPLODBIAS));
      device.SetSamplerState(sampler, D3DSAMP_MAXMIPLEVEL, at(D3DSAMP_MAXMIPLEVEL));
      device.SetSamplerState(sampler, D3DSAMP_MAXANISOTROPY, at(D3DSAMP_MAXANISOTROPY));
      device.SetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, at(D3DSAMP_SRGBTEXTURE));
      device.SetSamplerState(sampler, D3DSAMP_ELEMENTINDEX, at(D3DSAMP_ELEMENTINDEX));
      device.SetSamplerState(sampler, D3DSAMP_DMAPOFFSET, at(D3DSAMP_DMAPOFFSET));
   }

private:
   std::array<DWORD, 13> _state_block;
};

inline auto create_filled_sampler_state_block(IDirect3DDevice9& device, int sampler) noexcept
{
   Sampler_state_block block;

   device.GetSamplerState(sampler, D3DSAMP_ADDRESSU, &block.at(D3DSAMP_ADDRESSU));
   device.GetSamplerState(sampler, D3DSAMP_ADDRESSV, &block.at(D3DSAMP_ADDRESSV));
   device.GetSamplerState(sampler, D3DSAMP_ADDRESSW, &block.at(D3DSAMP_ADDRESSW));
   device.GetSamplerState(sampler, D3DSAMP_BORDERCOLOR, &block.at(D3DSAMP_BORDERCOLOR));
   device.GetSamplerState(sampler, D3DSAMP_MAGFILTER, &block.at(D3DSAMP_MAGFILTER));
   device.GetSamplerState(sampler, D3DSAMP_MINFILTER, &block.at(D3DSAMP_MINFILTER));
   device.GetSamplerState(sampler, D3DSAMP_MIPFILTER, &block.at(D3DSAMP_MIPFILTER));
   device.GetSamplerState(sampler, D3DSAMP_MIPMAPLODBIAS,
                          &block.at(D3DSAMP_MIPMAPLODBIAS));
   device.GetSamplerState(sampler, D3DSAMP_MAXMIPLEVEL, &block.at(D3DSAMP_MAXMIPLEVEL));
   device.GetSamplerState(sampler, D3DSAMP_MAXANISOTROPY,
                          &block.at(D3DSAMP_MAXANISOTROPY));
   device.GetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, &block.at(D3DSAMP_SRGBTEXTURE));
   device.GetSamplerState(sampler, D3DSAMP_ELEMENTINDEX,
                          &block.at(D3DSAMP_ELEMENTINDEX));
   device.GetSamplerState(sampler, D3DSAMP_DMAPOFFSET, &block.at(D3DSAMP_DMAPOFFSET));

   return block;
}

inline auto create_filled_sampler_state_blocks(IDirect3DDevice9& device) noexcept
   -> std::array<Sampler_state_block, 16>
{
   std::array<Sampler_state_block, 16> blocks;

   for (auto i = 0; i < 16; ++i) {
      blocks[i] = create_filled_sampler_state_block(device, i);
   }

   return blocks;
}
}
