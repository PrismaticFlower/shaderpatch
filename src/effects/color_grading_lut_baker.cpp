
#include "color_grading_lut_baker.hpp"
#include "../logger.hpp"
#include "color_helpers.hpp"
#include "tonemappers.hpp"
#include "utility.hpp"

#include <cstring>
#include <execution>
#include <vector>

#include <glm/gtc/packing.hpp>

namespace sp::effects {

auto Color_grading_lut_baker::srv() noexcept -> ID3D11ShaderResourceView*
{
   return _srv.get();
}

void Color_grading_lut_baker::bake_color_grading_lut(ID3D11DeviceContext1& dc,
                                                     const Color_grading_eval_params params) noexcept
{
   static Lut_data data{};

   std::for_each_n(std::execution::par_unseq, Index_iterator{}, lut_dimension, [&](int z) {
      for (int y = 0; y < lut_dimension; ++y) {
         for (int x = 0; x < lut_dimension; ++x) {
            glm::vec3 col{static_cast<float>(x), static_cast<float>(y),
                          static_cast<float>(z)};
            col /= static_cast<float>(lut_dimension) - 1.0f;

            col = logc_to_linear(col);
            col = apply_color_grading(col, params);

            data[z][y][x] = glm::packUnorm4x8({col, 1.0f});
         }
      }
   });

   uploade_lut(dc, data);
}

glm::vec3 Color_grading_lut_baker::apply_color_grading(
   glm::vec3 color, const Color_grading_eval_params& params) noexcept
{
   color *= params.color_filter;
   color = apply_basic_saturation(color, params.saturation);
   color = apply_hsv_adjust(color, params.hsv_adjust);
   color = apply_channel_mixing(color, params.channel_mix_red,
                                params.channel_mix_green, params.channel_mix_blue);
   color = apply_log_contrast(color, params.contrast);
   color = apply_lift_gamma_gain(color, params.lift_adjust,
                                 params.inv_gamma_adjust, params.gain_adjust);
   color = apply_tonemapping(color, params);

   color = linear_srgb_to_gamma_srgb(color);

   return color;
}

glm::vec3 Color_grading_lut_baker::apply_tonemapping(
   glm::vec3 color, const Color_grading_eval_params& params) noexcept
{
   if (params.tonemapper == Tonemapper::filmic) {
      color = filmic::eval(color, params.filmic_curve);
   }
   else if (params.tonemapper == Tonemapper::aces_fitted) {
      color = eval_aces_srgb_fitted(color);
   }
   else if (params.tonemapper == Tonemapper::filmic_heji2015) {
      color = eval_filmic_hejl2015(color, params.heji_whitepoint);
   }
   else if (params.tonemapper == Tonemapper::reinhard) {
      color = eval_reinhard(color);
   }

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
