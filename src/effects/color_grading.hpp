#pragma once

#include "../direct3d/shader_constant.hpp"
#include "com_ptr.hpp"

#include <array>
#include <initializer_list>
#include <vector>

#include <glm/glm.hpp>
#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

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

   auto params() const noexcept -> const Color_grading_params&
   {
      return _user_params;
   }

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

   bool _open_failure = false;
   bool _save_failure = false;
};

}

namespace YAML {
template<>
struct convert<sp::effects::Color_grading_params> {
   static Node encode(const sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      YAML::Node node;

      node["ColorFilter"s].push_back(params.color_filter.r);
      node["ColorFilter"s].push_back(params.color_filter.g);
      node["ColorFilter"s].push_back(params.color_filter.b);

      node["Saturation"s] = params.saturation;
      node["Exposure"s] = params.exposure;
      node["Contrast"s] = params.contrast;

      node["FilmicToeStrength"s] = params.filmic_toe_strength;
      node["FilmicToeLength"s] = params.filmic_toe_length;
      node["FilmicShoulderStrength"s] = params.filmic_shoulder_strength;
      node["FilmicShoulderLength"s] = params.filmic_shoulder_length;
      node["FilmicShoulderAngle"s] = params.filmic_shoulder_angle;

      node["ShadowColor"s].push_back(params.shadow_color.r);
      node["ShadowColor"s].push_back(params.shadow_color.g);
      node["ShadowColor"s].push_back(params.shadow_color.b);

      node["MidtoneColor"s].push_back(params.midtone_color.r);
      node["MidtoneColor"s].push_back(params.midtone_color.g);
      node["MidtoneColor"s].push_back(params.midtone_color.b);

      node["HighlightColor"s].push_back(params.highlight_color.r);
      node["HighlightColor"s].push_back(params.highlight_color.g);
      node["HighlightColor"s].push_back(params.highlight_color.b);

      node["ShadowOffset"s] = params.shadow_offset;
      node["MidtoneOffset"s] = params.midtone_offset;
      node["HighlightOffset"s] = params.highlight_offset;

      return node;
   }

   static bool decode(const Node& node, sp::effects::Color_grading_params& params)
   {
      using namespace std::literals;

      params = sp::effects::Color_grading_params{};

      params.color_filter.r = node["ColorFilter"s][0].as<float>();
      params.color_filter.g = node["ColorFilter"s][1].as<float>();
      params.color_filter.b = node["ColorFilter"s][2].as<float>();

      params.saturation = node["Saturation"s].as<float>();
      params.exposure = node["Exposure"s].as<float>();
      params.contrast = node["Contrast"s].as<float>();

      params.filmic_toe_strength = node["FilmicToeStrength"s].as<float>();
      params.filmic_toe_length = node["FilmicToeLength"s].as<float>();
      params.filmic_shoulder_strength = node["FilmicShoulderStrength"s].as<float>();
      params.filmic_shoulder_length = node["FilmicShoulderLength"s].as<float>();
      params.filmic_shoulder_angle = node["FilmicShoulderAngle"s].as<float>();

      params.shadow_color.r = node["ShadowColor"s][0].as<float>();
      params.shadow_color.g = node["ShadowColor"s][1].as<float>();
      params.shadow_color.b = node["ShadowColor"s][2].as<float>();

      params.midtone_color.r = node["MidtoneColor"s][0].as<float>();
      params.midtone_color.g = node["MidtoneColor"s][1].as<float>();
      params.midtone_color.b = node["MidtoneColor"s][2].as<float>();

      params.highlight_color.r = node["HighlightColor"s][0].as<float>();
      params.highlight_color.g = node["HighlightColor"s][1].as<float>();
      params.highlight_color.b = node["HighlightColor"s][2].as<float>();

      params.shadow_offset = node["ShadowOffset"s].as<float>();
      params.midtone_offset = node["MidtoneOffset"s].as<float>();
      params.highlight_offset = node["HighlightOffset"s].as<float>();

      return true;
   }
};
}
