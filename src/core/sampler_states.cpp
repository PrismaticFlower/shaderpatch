
#include "sampler_states.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"

#include <comdef.h>

namespace sp::core {

namespace {

auto make_sampler(ID3D11Device1& device, const D3D11_FILTER filter,
                  const D3D11_TEXTURE_ADDRESS_MODE address_mode,
                  const UINT max_anisotropy = 1,
                  std::array<float, 4> border_color = {0.0f, 0.0f, 0.0f, 0.0f}) noexcept
   -> Com_ptr<ID3D11SamplerState>
{
   Com_ptr<ID3D11SamplerState> sampler;

   const auto desc =
      D3D11_SAMPLER_DESC{.Filter = filter,
                         .AddressU = address_mode,
                         .AddressV = address_mode,
                         .AddressW = address_mode,
                         .MipLODBias = 0.0f,
                         .MaxAnisotropy = max_anisotropy,
                         .ComparisonFunc = D3D11_COMPARISON_NEVER,
                         .BorderColor = {border_color[0], border_color[1],
                                         border_color[2], border_color[3]},
                         .MinLOD = -FLT_MAX,
                         .MaxLOD = FLT_MAX};

   if (const auto result =
          device.CreateSamplerState(&desc, sampler.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create sampler state! reason: {}",
                        _com_error{result}.ErrorMessage());
   }

   return sampler;
}

}

Sampler_states::Sampler_states(ID3D11Device1& device) noexcept
   : linear_clamp_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                       D3D11_TEXTURE_ADDRESS_CLAMP)},
     linear_wrap_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                      D3D11_TEXTURE_ADDRESS_WRAP)},
     linear_mirror_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                        D3D11_TEXTURE_ADDRESS_MIRROR)},
     point_clamp_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_MIP_POINT,
                                      D3D11_TEXTURE_ADDRESS_CLAMP)},
     point_wrap_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_MIP_POINT,
                                     D3D11_TEXTURE_ADDRESS_WRAP)},
     text_sampler{make_sampler(device, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                               D3D11_TEXTURE_ADDRESS_BORDER, 1,
                               {0.0f, 0.0f, 0.0f, 0.0f})}
{
   _last_sample_count = to_sample_count(user_config.graphics.anisotropic_filtering);

   aniso_clamp_sampler = make_sampler(device, D3D11_FILTER_ANISOTROPIC,
                                      D3D11_TEXTURE_ADDRESS_CLAMP, _last_sample_count);
   aniso_wrap_sampler = make_sampler(device, D3D11_FILTER_ANISOTROPIC,
                                     D3D11_TEXTURE_ADDRESS_WRAP, _last_sample_count);
}

void Sampler_states::update_ansio_samplers(ID3D11Device1& device) noexcept
{
   const auto new_sample_count =
      to_sample_count(user_config.graphics.anisotropic_filtering);

   if (std::exchange(_last_sample_count, new_sample_count) == new_sample_count)
      return;

   if (user_config.graphics.anisotropic_filtering == Anisotropic_filtering::off) {
      aniso_clamp_sampler = make_sampler(device, D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                         D3D11_TEXTURE_ADDRESS_CLAMP);
      aniso_wrap_sampler = make_sampler(device, D3D11_FILTER_MIN_MAG_MIP_LINEAR,
                                        D3D11_TEXTURE_ADDRESS_WRAP);
   }
   else {
      aniso_clamp_sampler =
         make_sampler(device, D3D11_FILTER_ANISOTROPIC,
                      D3D11_TEXTURE_ADDRESS_CLAMP, _last_sample_count);
      aniso_wrap_sampler =
         make_sampler(device, D3D11_FILTER_ANISOTROPIC,
                      D3D11_TEXTURE_ADDRESS_WRAP, _last_sample_count);
   }
}

}
