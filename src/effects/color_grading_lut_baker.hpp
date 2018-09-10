#pragma once

#include "../texture.hpp"
#include "com_ref.hpp"
#include "filmic_tonemapper.hpp"
#include "postprocess_params.hpp"

#include <array>
#include <optional>
#include <typeinfo>

namespace sp::effects {

class Color_grading_lut_baker {
public:
   Color_grading_lut_baker(Com_ref<IDirect3DDevice9> device) noexcept
      : _device{std::move(device)}
   {
   }

   void drop_device_resources() noexcept;

   void update_params(const Color_grading_params& params) noexcept;

   void bind_texture(int slot) noexcept;

private:
   void bake_color_grading_lut() noexcept;

   glm::vec3 apply_color_grading(glm::vec3 color) noexcept;

   struct Eval_params {
      float contrast;
      float saturation;

      glm::vec3 color_filter;
      glm::vec3 hsv_adjust;

      glm::vec3 channel_mix_red;
      glm::vec3 channel_mix_green;
      glm::vec3 channel_mix_blue;

      glm::vec3 lift_adjust;
      glm::vec3 inv_gamma_adjust;
      glm::vec3 gain_adjust;

      filmic::Curve filmic_curve;
   };

   static_assert(std::is_trivially_destructible_v<Eval_params>);

   Com_ref<IDirect3DDevice9> _device;
   Eval_params _eval_params;
   std::optional<Texture> _texture;
};

}
