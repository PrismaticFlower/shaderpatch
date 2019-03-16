
#include "color_grading_lut_baker.hpp"
#include "../logger.hpp"
#include "color_helpers.hpp"
#include "utility.hpp"

#include <cstring>
#include <execution>
#include <vector>

#include <glm/gtc/packing.hpp>

namespace sp::effects {

using namespace std::literals;

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
      _rebake = true;
   }
}

auto Color_grading_lut_baker::srv(ID3D11DeviceContext1& dc) noexcept
   -> ID3D11ShaderResourceView*
{
   if (std::exchange(_rebake, false)) bake_color_grading_lut(dc);

   return _srv.get();
}

void Color_grading_lut_baker::bake_color_grading_lut(ID3D11DeviceContext1& dc) noexcept
{
   Lut_data data{};

   std::for_each_n(std::execution::par_unseq, Index_iterator{}, lut_dimension, [&](int z) {
      for (int x = 0; x < lut_dimension; ++x) {
         for (int y = 0; y < lut_dimension; ++y) {
            glm::vec3 col{static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(z)};
            col /= static_cast<float>(lut_dimension) - 1.0f;

            col = logc_to_linear(col);
            col = apply_color_grading(col);

            data[z][y][x] = glm::packUnorm4x8({col, 1.0f});
         }
      }
   });

   uploade_lut(dc, data);
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

   color = linear_to_srgb(color);

   return color;
}

void Color_grading_lut_baker::uploade_lut(ID3D11DeviceContext1& dc,
                                          const Lut_data& lut_data) noexcept
{
   dc.UpdateSubresource(_texture.get(), 0, nullptr, &lut_data,
                        sizeof(Lut_data::value_type::value_type),
                        sizeof(Lut_data::value_type));
}

}
