
#include "color_grading_lut_baker.hpp"
#include "../logger.hpp"
#include "algorithm.hpp"
#include "color_helpers.hpp"

#include <cstring>
#include <execution>
#include <vector>

#include <glm/gtc/packing.hpp>

namespace sp::effects {

using namespace std::literals;

namespace {

constexpr auto lut_dimension = 32;

using Lut_data =
   std::array<std::array<std::array<glm::uint64, lut_dimension>, lut_dimension>, lut_dimension>;

constexpr Sampler_info lut_sampler_info = {D3DTADDRESS_CLAMP,
                                           D3DTADDRESS_CLAMP,
                                           D3DTADDRESS_CLAMP,
                                           D3DTEXF_LINEAR,
                                           D3DTEXF_LINEAR,
                                           D3DTEXF_NONE,
                                           0.0f,
                                           false};

auto create_sp_lut(IDirect3DDevice9& device, IDirect3DVolumeTexture9& d3d_texture) -> Texture
{
   Com_ptr<IDirect3DBaseTexture9> base_texture;
   d3d_texture.QueryInterface(IID_IDirect3DBaseTexture9,
                              base_texture.void_clear_and_assign());

   device.AddRef();

   return Texture{Com_ptr{&device}, std::move(base_texture), lut_sampler_info};
}

auto create_d3d_system_lut(IDirect3DDevice9& device, const Lut_data& lut_data) noexcept
   -> Com_ptr<IDirect3DVolumeTexture9>
{
   Com_ptr<IDirect3DVolumeTexture9> texture;

   const auto result =
      device.CreateVolumeTexture(lut_dimension, lut_dimension, lut_dimension, 1,
                                 0, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM,
                                 texture.clear_and_assign(), nullptr);

   if (FAILED(result)) {
      log_and_terminate("Failed to create LUT for colour grading."s);
   }

   D3DLOCKED_BOX box;

   texture->LockBox(0, &box, nullptr, 0);

   Ensures(sizeof(lut_data) == box.SlicePitch * lut_dimension);

   std::memcpy(box.pBits, &lut_data, sizeof(lut_data));

   texture->UnlockBox(0);

   return texture;
}

auto create_d3d_gpu_lut(IDirect3DDevice9& device,
                        IDirect3DVolumeTexture9& system_texture) noexcept
   -> Com_ptr<IDirect3DVolumeTexture9>
{
   Com_ptr<IDirect3DVolumeTexture9> gpu_texture;

   const auto result =
      device.CreateVolumeTexture(lut_dimension, lut_dimension, lut_dimension, 1,
                                 0, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT,
                                 gpu_texture.clear_and_assign(), nullptr);

   if (FAILED(result)) {
      log_and_terminate("Failed to create LUT for colour grading."s);
   }

   device.UpdateTexture(&system_texture, gpu_texture.get());

   return gpu_texture;
}

auto create_lut(IDirect3DDevice9& device, const Lut_data& lut_data) noexcept -> Texture
{
   auto system_texture = create_d3d_system_lut(device, lut_data);
   auto gpu_texture = create_d3d_gpu_lut(device, *system_texture);

   return create_sp_lut(device, *gpu_texture);
}

}

void Color_grading_lut_baker::drop_device_resources() noexcept
{
   _texture = std::nullopt;
}

void Color_grading_lut_baker::update_params(const Color_grading_params& params) noexcept
{
   Eval_params eval;

   eval.saturation = params.saturation;
   eval.contrast = params.contrast;

   eval.color_filter = params.color_filter;

   eval.hsv_adjust.x = params.hsv_hue_adjustment;
   eval.hsv_adjust.y = params.hsv_saturation_adjustment;
   eval.hsv_adjust.z = params.hsv_value_adjustment;

   eval.channel_mix_red = params.channel_mix_red;
   eval.channel_mix_green = params.channel_mix_green;
   eval.channel_mix_blue = params.channel_mix_blue;

   auto lift = params.shadow_color;
   lift -= (lift.x + lift.y + lift.z) / 3.0f;

   auto gamma = params.midtone_color;
   gamma -= (gamma.x + gamma.y + gamma.z) / 3.0f;

   auto gain = params.highlight_color;
   gain -= (gain.x + gain.y + gain.z) / 3.0f;

   eval.lift_adjust = 0.0f + (lift + params.shadow_offset);
   eval.gain_adjust = 1.0f + (gain + params.highlight_offset);

   const auto mid_grey = 0.5f + (gamma + params.midtone_offset);

   eval.inv_gamma_adjust =
      1.0f / (glm::log(0.5f - eval.lift_adjust) /
              (eval.gain_adjust - eval.lift_adjust) / glm::log(mid_grey));

   eval.filmic_curve = filmic::color_grading_params_to_curve(params);

   static_assert(std::is_same_v<decltype(eval), decltype(_eval_params)>);
   static_assert(std::is_trivially_destructible_v<decltype(eval)>);

   if (std::memcmp(&eval, &_eval_params, sizeof(Eval_params)) != 0) {
      _eval_params = eval;
      _texture = std::nullopt;
   }
}

void Color_grading_lut_baker::bind_texture(int slot) noexcept
{
   if (!_texture) bake_color_grading_lut();

   _texture->bind(slot);
}

void Color_grading_lut_baker::bake_color_grading_lut() noexcept
{
   Lut_data data{};

   for_each(std::execution::par_unseq, index_array<lut_dimension>, [&](int x) {
      for (int y = 0; y < lut_dimension; ++y) {
         for (int z = 0; z < lut_dimension; ++z) {
            glm::vec3 col{static_cast<float>(z), static_cast<float>(y),
                          static_cast<float>(x)};
            col /= static_cast<float>(lut_dimension) - 1.0f;

            col = logc_to_linear(col);
            col = apply_color_grading(col);

            data[x][y][z] = glm::packHalf4x16({col, 1.0f});
         }
      }
   });

   _texture = create_lut(*_device, data);
}

glm::vec3 Color_grading_lut_baker::apply_color_grading(glm::vec3 color) noexcept
{

   color *= _eval_params.color_filter;
   color = apply_basic_saturation(color, _eval_params.saturation);
   color = apply_hsv_adjust(color, _eval_params.hsv_adjust);
   color = apply_channel_mixing(color, _eval_params.channel_mix_red,
                                _eval_params.channel_mix_green,
                                _eval_params.channel_mix_blue);
   color = apply_log_contrast(color, _eval_params.contrast);
   color = apply_lift_gamma_gain(color, _eval_params.lift_adjust,
                                 _eval_params.inv_gamma_adjust,
                                 _eval_params.gain_adjust);
   color = filmic::eval(color, _eval_params.filmic_curve);

   return color;
}

}
