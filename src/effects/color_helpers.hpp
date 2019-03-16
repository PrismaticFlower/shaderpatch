#pragma once

#include <cmath>

#include <glm/glm.hpp>

namespace sp::effects {

namespace logc {
constexpr auto cut = 0.011361f;
constexpr auto a = 5.555556f;
constexpr auto b = 0.047996f;
constexpr auto c = 0.244161f;
constexpr auto d = 0.386036f;
constexpr auto e = 5.301883f;
constexpr auto f = 0.092814f;
}

inline glm::vec3 linear_to_logc(glm::vec3 color) noexcept
{
   color = logc::a * color + logc::b;

   color.r = log10(color.r);
   color.g = log10(color.g);
   color.b = log10(color.b);

   return logc::c * color + logc::d;
}

inline glm::vec3 logc_to_linear(glm::vec3 color) noexcept
{
   return (glm::pow(glm::vec3{10.0f}, (color - logc::d) / logc::c) - logc::b) /
          logc::a;
}

inline glm::vec3 rgb_to_hsv(glm::vec3 rgb) noexcept
{
   float k = 0.0f;

   if (rgb.g < rgb.b) {
      std::swap(rgb.g, rgb.b);
      k = -1.0f;
   }

   if (rgb.r < rgb.g) {
      std::swap(rgb.r, rgb.g);
      k = -2.0f / 6.0f - k;
   }

   const auto chroma = rgb.r - (rgb.g < rgb.b ? rgb.g : rgb.b);

   glm::vec3 hsv;

   hsv.x = glm::abs(k + (rgb.g - rgb.b) / (6.0f * chroma + 1e-20f));
   hsv.y = chroma / (rgb.r + 1e-20f);
   hsv.z = rgb.r;

   return hsv;
}

inline glm::vec3 hsv_to_rgb(glm::vec3 hsv)
{
   glm::vec3 rgb;

   rgb.r = glm::abs(hsv.x * 6.0f - 3.0f) - 1.0f;
   rgb.g = 2.0f - glm::abs(hsv.x * 6.0f - 2.0f);
   rgb.b = 2.0f - glm::abs(hsv.x * 6.0f - 4.0f);

   rgb = glm::clamp(rgb, 0.0f, 1.0f);

   return ((rgb - 1.0f) * hsv.y + 1.0f) * hsv.z;
}

inline float linear_to_srgb(const float v) noexcept
{
   return v <= 0.0031308f ? 12.92f * v : 1.055f * pow(v, 1.0f / 2.4f) - 0.055f;
}

inline glm::vec3 linear_to_srgb(const glm::vec3 color) noexcept
{
   return {linear_to_srgb(color.r), linear_to_srgb(color.g),
           linear_to_srgb(color.b)};
}

inline glm::vec3 apply_basic_saturation(glm::vec3 color, float saturation) noexcept
{
   const auto saturation_weights = glm::vec3{0.25f, 0.5f, 0.25f};

   float grey = glm::dot(saturation_weights, color);
   color = grey + (saturation * (color - grey));

   return color;
}

inline glm::vec3 apply_hsv_adjust(glm::vec3 color, glm::vec3 hsv_adjust) noexcept
{
   glm::vec3 hsv = rgb_to_hsv(color);

   hsv.x = glm::clamp(hsv.x + hsv_adjust.x, 0.0f, 1.0f);
   hsv.y = glm::clamp(hsv.y * hsv_adjust.y, 0.0f, 1.0f);
   hsv.z *= hsv_adjust.z;

   return hsv_to_rgb(hsv);
}

inline glm::vec3 apply_log_contrast(glm::vec3 color, float contrast) noexcept
{
   const auto contrast_midpoint = glm::log2(.18f);
   constexpr auto contrast_epsilon = 1e-5f;

   glm::vec3 log_col = glm::log2(color + contrast_epsilon);
   glm::vec3 adj_col = contrast_midpoint + (log_col - contrast_midpoint) * contrast;

   return glm::max(glm::vec3{0.0f}, glm::exp2(adj_col) - contrast_epsilon);
}

inline glm::vec3 apply_channel_mixing(glm::vec3 color, glm::vec3 mix_red,
                                      glm::vec3 mix_green, glm::vec3 mix_blue) noexcept
{
   return glm::vec3{glm::dot(color, mix_red), glm::dot(color, mix_green),
                    glm::dot(color, mix_blue)};
}

inline glm::vec3 apply_lift_gamma_gain(glm::vec3 color, glm::vec3 lift_adjust,
                                       glm::vec3 inv_gamma_adjust,
                                       glm::vec3 gain_adjust) noexcept
{
   color = color * gain_adjust + lift_adjust;

   return glm::sign(color) * glm::pow(glm::abs(color), inv_gamma_adjust);
}

}
