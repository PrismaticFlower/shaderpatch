#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Sampler_states {
   Sampler_states(ID3D11Device1& device) noexcept;

   const Com_ptr<ID3D11SamplerState> aniso_clamp_sampler;
   const Com_ptr<ID3D11SamplerState> aniso_wrap_sampler;
   const Com_ptr<ID3D11SamplerState> linear_clamp_sampler;
   const Com_ptr<ID3D11SamplerState> linear_wrap_sampler;
   const Com_ptr<ID3D11SamplerState> point_clamp_sampler;
   const Com_ptr<ID3D11SamplerState> point_wrap_sampler;
};

}
