#pragma once

#include "postprocess_params.hpp"

#include <glm/glm.hpp>
#include <imgui.h>

// This is almost entirely John Hable's work explained here,
// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// used under CC0 1.0 Universal. Big thanks go out to him for providing such
// useful material and references for it.
namespace sp::effects::filmic {

struct Curve_segment {
   float offset_x = 0.0f;
   float offset_y = 0.0f;
   float scale_x = 1.0f; // always 1 or -1
   float scale_y = 0.0f;
   float ln_a = 0.0f;
   float b = 1.0f;
};

struct Curve {
   float w = 1.0f;
   float inv_w = 1.0f;

   float x0 = 0.25f;
   float y0 = 0.25f;
   float x1 = 0.75f;
   float y1 = 0.75f;

   std::array<Curve_segment, 3> segments{};
};

float eval_segment(float v, const Curve_segment& segm);

inline void as_slope_intercept(float& m, float& b, float x0, float x1, float y0, float y1)
{
   float dy = (y1 - y0);
   float dx = (x1 - x0);
   m = (dx == 0) ? 1.0f : (dy / dx);

   b = y0 - x0 * m;
}

inline float eval_derivative_linear_gamma(float m, float b, float g, float x)
{
   return g * m * glm::pow(m * x + b, g - 1.0f);
}

inline void solve_ab(float& ln_a, float& b, float x0, float y0, float m)
{
   b = (m * x0) / y0;
   ln_a = glm::log(y0) - b * glm::log(x0);
}

inline Curve get_eval_curve_params(float x0, float y0, float x1, float y1, float w,
                                   float overshoot_x, float overshoot_y) noexcept
{
   constexpr float gamma = 1.0f;

   Curve curve;

   curve.w = w;
   curve.inv_w = 1.0f / w;

   // normalize params to 1.0 range
   x0 /= w;
   x1 /= w;
   overshoot_x /= w;

   float toe_mid = 0.0f;
   float shoulder_mid = 0.0f;

   {
      float m, b;
      as_slope_intercept(m, b, x0, x1, y0, y1);

      constexpr float g = 1.0f;

      Curve_segment mid_segment;
      mid_segment.offset_x = -(b / m);
      mid_segment.offset_y = 0.0f;
      mid_segment.scale_x = 1.0f;
      mid_segment.scale_y = 1.0f;
      mid_segment.ln_a = g * glm::log(m);
      mid_segment.b = g;

      curve.segments[1] = mid_segment;

      toe_mid = eval_derivative_linear_gamma(m, b, g, x0);
      shoulder_mid = eval_derivative_linear_gamma(m, b, g, x1);

      // apply gamma to endpoints
      y0 = glm::max(1e-5f, glm::pow(y0, gamma));
      y1 = glm::max(1e-5f, glm::pow(y1, gamma));

      overshoot_y = glm::pow(1.0f + overshoot_y, gamma) - 1.0f;
   }

   curve.x0 = x0;
   curve.x1 = x1;
   curve.y0 = y0;
   curve.y1 = y1;

   // toe section
   {
      Curve_segment toe_segment;
      toe_segment.offset_x = 0;
      toe_segment.offset_y = 0.0f;
      toe_segment.scale_x = 1.0f;
      toe_segment.scale_y = 1.0f;

      solve_ab(toe_segment.ln_a, toe_segment.b, x0, y0, toe_mid);
      curve.segments[0] = toe_segment;
   }

   // shoulder section
   {
      // use the simple version that is usually too flat
      Curve_segment shoulder_segment;

      float x0_shoulder = (1.0f + overshoot_x) - x1;
      float y0_shoulder = (1.0f + overshoot_y) - y1;

      float ln_a = 0.0f;
      float b = 0.0f;
      solve_ab(ln_a, b, x0_shoulder, y0_shoulder, shoulder_mid);

      shoulder_segment.offset_x = (1.0f + overshoot_x);
      shoulder_segment.offset_y = (1.0f + overshoot_y);

      shoulder_segment.scale_x = -1.0f;
      shoulder_segment.scale_y = -1.0f;
      shoulder_segment.ln_a = ln_a;
      shoulder_segment.b = b;

      curve.segments[2] = shoulder_segment;
   }

   // Normalize so that we hit 1.0 at our white point. We wouldn't have do
   // this if we skipped the overshoot part.
   {
      // evaluate shoulder at the end of the curve
      float scale = eval_segment(1.0f, curve.segments[2]);
      float inv_scale = 1.0f / scale;

      curve.segments[0].offset_y *= inv_scale;
      curve.segments[0].scale_y *= inv_scale;

      curve.segments[1].offset_y *= inv_scale;
      curve.segments[1].scale_y *= inv_scale;

      curve.segments[2].offset_y *= inv_scale;
      curve.segments[2].scale_y *= inv_scale;
   }

   return curve;
}

inline Curve color_grading_params_to_curve(const Color_grading_params& params) noexcept
{
   const float toe_strength = glm::clamp(params.filmic_toe_strength, 0.0f, 1.0f);
   const float toe_length = glm::clamp(params.filmic_toe_length, 0.0f, 1.0f);
   const float shoulder_strength = glm::max(params.filmic_shoulder_strength, 0.0f);
   const float shoulder_length = glm::clamp(params.filmic_shoulder_length, 0.0f, 1.0f);
   const float shoulder_angle = glm::clamp(params.filmic_shoulder_angle, 0.0f, 1.0f);

   // This is not actually the display gamma. It's just a UI space to avoid
   // having to enter small numbers for the input.
   float perceptual_gamma = 2.2f;

   // apply base params

   // toe goes from 0 to 0.5
   float x0 = toe_length * .5f;
   float y0 = (1.0f - toe_strength) * x0; // lerp from 0 to x0

   float remaining_y = 1.0f - y0;

   float initial_w = x0 + remaining_y;

   float y1_offset = (1.0f - shoulder_length) * remaining_y;
   float x1 = x0 + y1_offset;
   float y1 = y0 + y1_offset;

   // filmic shoulder strength is in F stops
   float extra_w = glm::exp2(shoulder_strength) - 1.0f;

   float w = initial_w + extra_w;

   // to adjust the perceptual gamma space, apply power
   x0 = glm::pow(x0, perceptual_gamma);
   y0 = glm::pow(y0, perceptual_gamma);
   x1 = glm::pow(x1, perceptual_gamma);
   y1 = glm::pow(y1, perceptual_gamma);

   float overshoot_x = (w * 2.0f) * shoulder_angle * shoulder_strength;
   float overshoot_y = 0.5f * shoulder_angle * shoulder_strength;

   return get_eval_curve_params(x0, y0, x1, y1, w, overshoot_x, overshoot_y);
}

inline float eval_segment(float v, const Curve_segment& segm)
{
   const float x0 = (v - segm.offset_x) * segm.scale_x;
   float y0 = 0.0f;

   if (x0 > 0) y0 = glm::exp(segm.ln_a + segm.b * glm::log(x0));

   return y0 * segm.scale_y + segm.offset_y;
}

inline float eval(float v, const Curve& curve)
{
   const float norm_v = v * curve.inv_w;
   const int index = (norm_v < curve.x0) ? 0 : ((norm_v < curve.x1) ? 1 : 2);

   return eval_segment(norm_v, curve.segments[index]);
}

inline glm::vec3 eval(glm::vec3 v, const Curve& curve)
{
   v.r = eval(v.r, curve);
   v.g = eval(v.g, curve);
   v.b = eval(v.b, curve);

   return v;
}

}
