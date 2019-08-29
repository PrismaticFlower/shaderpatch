#pragma once

#include "filmic_tonemapper.hpp"
#include "postprocess_params.hpp"

#include <glm/glm.hpp>

namespace sp::effects {

struct Color_grading_eval_params {
   Color_grading_eval_params() = default;

   explicit Color_grading_eval_params(const Color_grading_params& params) noexcept
   {
      saturation = params.saturation;
      contrast = params.contrast;

      color_filter = params.color_filter;

      hsv_adjust.x = params.hsv_hue_adjustment;
      hsv_adjust.y = params.hsv_saturation_adjustment;
      hsv_adjust.z = params.hsv_value_adjustment;

      channel_mix_red = params.channel_mix_red;
      channel_mix_green = params.channel_mix_green;
      channel_mix_blue = params.channel_mix_blue;

      auto lift = params.shadow_color;
      lift -= (lift.x + lift.y + lift.z) / 3.0f;

      auto gamma = params.midtone_color;
      gamma -= (gamma.x + gamma.y + gamma.z) / 3.0f;

      auto gain = params.highlight_color;
      gain -= (gain.x + gain.y + gain.z) / 3.0f;

      lift_adjust = 0.0f + (lift + params.shadow_offset);
      gain_adjust = 1.0f + (gain + params.highlight_offset);

      const auto mid_grey = 0.5f + (gamma + params.midtone_offset);

      inv_gamma_adjust = 1.0f / (glm::log(0.5f - lift_adjust) /
                                 (gain_adjust - lift_adjust) / glm::log(mid_grey));

      tonemapper = params.tonemapper;

      filmic_curve = filmic::color_grading_params_to_curve(params);

      heji_whitepoint = params.filmic_heji_whitepoint;
   }

   float contrast = 1.0f;
   float saturation = 1.0f;

   glm::vec3 color_filter = {1.0f, 1.0f, 1.0f};
   glm::vec3 hsv_adjust = {0.0f, 0.0f, 0.0f};

   glm::vec3 channel_mix_red = {1.0f, 0.0f, 0.0f};
   glm::vec3 channel_mix_green = {0.0f, 1.0f, 0.0f};
   glm::vec3 channel_mix_blue = {0.0f, 0.0f, 1.0f};

   glm::vec3 lift_adjust = {0.0f, 0.0f, 0.0f};
   glm::vec3 inv_gamma_adjust = {0.0f, 0.0f, 0.0f};
   glm::vec3 gain_adjust = {0.0f, 0.0f, 0.0f};

   Tonemapper tonemapper = Tonemapper::none;
   filmic::Curve filmic_curve{};
   float heji_whitepoint = 1.0f;
};

static_assert(std::is_trivially_destructible_v<Color_grading_eval_params>);

}