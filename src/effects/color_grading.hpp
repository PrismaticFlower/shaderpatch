#pragma once

#include "../direct3d/shader_constant.hpp"
#include "com_ptr.hpp"

#include <array>
#include <initializer_list>
#include <vector>

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <d3d9.h>

namespace sp::effects {

struct Color_grading_params {
   glm::vec3 color_filter = {1.0f, 1.0f, 1.0f};
   float saturation = 1.0f;
   float exposure = 0.0f;
   float contrast = 1.0f;

   float filmic_toe_strength = 0.0f;
   float filmic_toe_length = 0.5f;
   float filmic_shoulder_strength = 0.0f;
   float filmic_shoulder_length = 0.5f;
   float filmic_shoulder_angle = 0.0f;

   bool srgb = true;

   glm::vec3 shadow_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 midtone_color = {1.0f, 1.0f, 1.0f};
   glm::vec3 highlight_color = {1.0f, 1.0f, 1.0f};

   float shadow_offset = 0.0f;
   float midtone_offset = 0.0f;
   float highlight_offset = 0.0f;
};

struct Filmic_curve_segment {
   float offset_x = 0.0f;
   float offset_y = 0.0f;
   float scale_x = 1.0f; // always 1 or -1
   float scale_y = 0.0f;
   float ln_a = 0.0f;
   float b = 1.0f;
};

struct Filmic_curve {
   float w = 1.0f;
   float inv_w = 1.0f;

   float x0 = 0.25f;
   float y0 = 0.25f;
   float x1 = 0.75f;
   float y1 = 0.75f;

   std::array<Filmic_curve_segment, 3> segments{};
};

class Color_grading {
public:
   Color_grading(gsl::not_null<Com_ptr<IDirect3DDevice9>> device)
      : _device(std::move(device))
   {
      set_params(Color_grading_params{});
   }

   void set_params(const Color_grading_params& params) noexcept;

   void bind_lut(int r_slot, int g_slot, int b_slot) noexcept;

   template<DWORD filter_offset, DWORD saturation_offset>
   void bind_constants() noexcept
   {
      direct3d::Ps_3f_shader_constant<filter_offset>{}.set(*_device,
                                                           _color_filter_exposure);
      direct3d::Ps_1f_shader_constant<saturation_offset>{}.set(*_device, _saturation);
   }

   void drop_device_resources() noexcept;

   void show_imgui() noexcept;

private:
   void update_eval_params() noexcept;

   void bake_lut() noexcept;

   void create_lut(const std::vector<glm::uint8>& texels,
                   Com_ptr<IDirect3DTexture9>& texture) noexcept;

   void upload_lut(const std::vector<glm::uint8>& texels,
                   IDirect3DTexture9& texture) noexcept;

   void setup_lut_samplers(std::initializer_list<int> indices) const noexcept;

   const Com_ptr<IDirect3DDevice9> _device;

   Com_ptr<IDirect3DTexture9> _r_texture;
   Com_ptr<IDirect3DTexture9> _g_texture;
   Com_ptr<IDirect3DTexture9> _b_texture;

   bool _lut_dirty = true;
   bool _use_dynamic_textures = false;
   bool _srgb = true;

   glm::vec3 _color_filter_exposure = {1.0f, 1.0f, 1.0f};

   const glm::vec3 _luminance_weights = {.25f, .5f, .25f};
   float _saturation = 1.0f;

   float _contrast_strength = 1.0f;
   const float _contrast_log_midpoint = glm::log2(.18f);
   const float _contrast_epsilon = 1e-5f;

   Filmic_curve _filmic_curve{};

   glm::vec3 _lift_adjust = {0.0f, 0.0f, 0.0f};
   glm::vec3 _inv_gamma_adjust = {1.0f, 1.0f, 1.0f};
   glm::vec3 _gain_adjust = {1.0f, 1.0f, 1.0f};

   float _max_table_value = 1.0f;

   const int _curve_size = 256;

   Color_grading_params _user_params{};
};

}
