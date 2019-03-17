#pragma once

#include <glm/glm.hpp>

namespace sp::effects {

inline auto eval_aces_srgb_fitted(glm::vec3 color) -> glm::vec3
{
   // Stephen Hill's ACES sRGB method.

   // clang-format off
   const glm::mat3 aces_input_mat{0.59719f, 0.35458f, 0.04823f,
                                  0.07600f, 0.90834f, 0.01566f,
                                  0.02840f, 0.13383f, 0.83777f};

   const glm::mat3 aces_output_mat{ 1.60475f, -0.53108f, -0.07367f,
                                   -0.10208f,  1.10813f, -0.00605f,
                                   -0.00327f, -0.07276f,  1.07602f};

   // clang-format on

   color = color * aces_input_mat;

   // RRT and ODT fit
   {
      const auto a = color * (color + 0.0245786f) - 0.000090537f;
      const auto b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
      color = a / b;
   }

   color = color * aces_output_mat;

   color = glm::clamp(color, 0.0f, 1.0f);

   color *= 1.8f;

   return color;
}

inline auto eval_filmic_hejl2015(const glm::vec3 color, const float whitepoint) noexcept
   -> glm::vec3
{
   const auto vh = glm::vec4{color, whitepoint};
   const auto va = (1.425f * vh) + 0.05f;
   const auto vf = ((vh * va + 0.004f) / (vh * (va + 0.55f) + 0.0491f)) - 0.0821f;

   return glm::vec3{vf.r, vf.g, vf.b} / vf.w;
}

inline auto eval_reinhard(const glm::vec3 color) noexcept -> glm::vec3
{
   return color / (color + 1.0f);
}
}
