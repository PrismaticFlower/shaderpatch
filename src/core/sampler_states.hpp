#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Sampler_states {
   Sampler_states(ID3D11Device1& device) noexcept;

   Com_ptr<ID3D11SamplerState> aniso_clamp_sampler;
   Com_ptr<ID3D11SamplerState> aniso_wrap_sampler;
   const Com_ptr<ID3D11SamplerState> linear_clamp_sampler;
   const Com_ptr<ID3D11SamplerState> linear_wrap_sampler;
   const Com_ptr<ID3D11SamplerState> point_clamp_sampler;
   const Com_ptr<ID3D11SamplerState> point_wrap_sampler;

   void update_ansio_samplers(ID3D11Device1& device) noexcept;

private:
   std::size_t _last_sample_count;
};

}
