
#include "color_grading.hpp"
#include "../imgui/imgui.h"
#include "../logger.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>

#include <glm/gtc/packing.hpp>
#include <gsl/gsl>

namespace sp::effects {

using namespace std::literals;

namespace {

// This is almost entirely John Hable's work explained here,
// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// used under CC0 1.0 Universal. Big thanks go out to him for providing such
// useful material and references for it.
namespace filmic {
float eval_segment(float v, const Filmic_curve_segment& segm);

void as_slope_intercept(float& m, float& b, float x0, float x1, float y0, float y1)
{
   float dy = (y1 - y0);
   float dx = (x1 - x0);
   m = (dx == 0) ? 1.0f : (dy / dx);

   b = y0 - x0 * m;
}

float eval_derivative_linear_gamma(float m, float b, float g, float x)
{
   return g * m * glm::pow(m * x + b, g - 1.0f);
}

void solve_ab(float& ln_a, float& b, float x0, float y0, float m)
{
   b = (m * x0) / y0;
   ln_a = glm::log(y0) - b * glm::log(x0);
}

Filmic_curve get_eval_curve_params(float x0, float y0, float x1, float y1, float w,
                                   float overshoot_x, float overshoot_y) noexcept
{
   constexpr float gamma = 1.0f;

   Filmic_curve curve;

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

      Filmic_curve_segment mid_segment;
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
      Filmic_curve_segment toe_segment;
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
      Filmic_curve_segment shoulder_segment;

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

   // Normalize so that we hit 1.0 at our white point. We wouldn't have do this
   // if we skipped the overshoot part.
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

Filmic_curve color_grading_params_to_curve(const Color_grading_params& params) noexcept
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

float eval_segment(float v, const Filmic_curve_segment& segm)
{
   const float x0 = (v - segm.offset_x) * segm.scale_x;
   float y0 = 0.0f;

   if (x0 > 0) y0 = glm::exp(segm.ln_a + segm.b * glm::log(x0));

   return y0 * segm.scale_y + segm.offset_y;
}

float eval(float v, const Filmic_curve& curve)
{
   const float norm_v = v * curve.inv_w;
   const int index = (norm_v < curve.x0) ? 0 : ((norm_v < curve.x1) ? 1 : 2);

   return eval_segment(norm_v, curve.segments[index]);
}

glm::vec3 eval(glm::vec3 v, const Filmic_curve& curve)
{
   v.r = eval(v.r, curve);
   v.g = eval(v.g, curve);
   v.b = eval(v.b, curve);

   return v;
}

void show_imgui_curve(const Color_grading_params& params)
{
   auto curve = color_grading_params_to_curve(params);
   void* data = &curve;

   const auto plotter = [](void* data, int idx) {
      const auto& curve = *static_cast<Filmic_curve*>(data);

      float v = idx / 256.0f;

      return eval(v, curve);
   };

   ImGui::PlotLines("Tonemap Curve", plotter, data, 1024);
}

}

glm::vec3 eval_log_contrast(glm::vec3 v, float eps, float log_midpoint, float contrast)
{
   glm::vec3 log_v = glm::log2(v + eps);
   glm::vec3 adj_v = log_midpoint + (log_v - log_midpoint) * contrast;

   return glm::max(glm::vec3{0.0f}, glm::exp2(adj_v) - eps);
}

float eval_log_contrast_reverse(float v, float eps, float log_midpoint, float contrast)
{
   float log_v = glm::log2(v + eps);
   float adj_v = (log_v - log_midpoint) / contrast + log_midpoint;

   return glm::max(0.0f, glm::exp2(adj_v) - eps);
}

glm::vec3 eval_lift_gamma_gain(glm::vec3 v, glm::vec3 lift_adjust,
                               glm::vec3 inv_gamma_adjust, glm::vec3 gain_adjust)
{
   v = glm::clamp((glm::pow(v, inv_gamma_adjust)), 0.0f, 1.0f);

   return gain_adjust * v + lift_adjust * (1.0f - v);
}

float linear_to_srgb(float v) noexcept
{
   v = glm::clamp(v, 0.0f, 1.0f);

   if (v <= 0.0031308f) {
      return 12.92f * v;
   }
   else {
      return 1.055f * pow(v, 1.0f / 2.4f) - 0.055f;
   }
}

glm::vec3 linear_to_srgb(glm::vec3 v) noexcept
{
   v.r = linear_to_srgb(v.r);
   v.g = linear_to_srgb(v.g);
   v.b = linear_to_srgb(v.b);

   return v;
}

}

void Color_grading::params(const Color_grading_params& params) noexcept
{
   _user_params = params;

   update_eval_params();
}

void Color_grading::drop_device_resources() noexcept
{
   _lut_dirty = true;
   _r_texture = nullptr;
   _g_texture = nullptr;
   _b_texture = nullptr;
}

void Color_grading::show_imgui() noexcept
{
   if (!std::exchange(_use_dynamic_textures, true)) drop_device_resources();

   ImGui::Begin("Colour Grading", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   const auto max_float = std::numeric_limits<float>::max();

   if (ImGui::CollapsingHeader("Basic Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorEdit3("Colour Filter", &_user_params.color_filter.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Exposure", &_user_params.exposure, 0.01f);
      ImGui::DragFloat("Saturation", &_user_params.saturation, 0.01f, 0.0f, max_float);
      ImGui::DragFloat("Contrast", &_user_params.contrast, 0.01f, 0.01f, max_float);
   }

   if (ImGui::CollapsingHeader("Filmic Tonemapping")) {
      filmic::show_imgui_curve(_user_params);

      ImGui::DragFloat("Toe Strength", &_user_params.filmic_toe_strength, 0.01f,
                       0.0f, 1.0f);
      ImGui::DragFloat("Toe Length", &_user_params.filmic_toe_length, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Strength", &_user_params.filmic_shoulder_strength,
                       0.01f, 0.0f, max_float);
      ImGui::DragFloat("Shoulder Length", &_user_params.filmic_shoulder_length,
                       0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Angle", &_user_params.filmic_shoulder_angle,
                       0.01f, 0.0f, 1.0f);

      if (ImGui::Button("Reset To Linear")) {
         _user_params.filmic_toe_strength = 0.0f;
         _user_params.filmic_toe_length = 0.5f;
         _user_params.filmic_shoulder_strength = 0.0f;
         _user_params.filmic_shoulder_length = 0.5f;
         _user_params.filmic_shoulder_angle = 0.0f;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip("Reset the curve to linear values.");
      }

      ImGui::SameLine();

      if (ImGui::Button("Reset To Example")) {
         _user_params.filmic_toe_strength = 0.5f;
         _user_params.filmic_toe_length = 0.5f;
         _user_params.filmic_shoulder_strength = 2.0f;
         _user_params.filmic_shoulder_length = 0.5f;
         _user_params.filmic_shoulder_angle = 1.0f;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip("Reset the curve to example starting point values.");
      }
   }

   if (ImGui::CollapsingHeader("Lift / Gamma / Gain")) {
      ImGui::ColorEdit3("Shadow Colour", &_user_params.shadow_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Shadow Offset", &_user_params.shadow_offset, 0.005f);

      ImGui::ColorEdit3("Midtone Colour", &_user_params.midtone_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Midtone Offset", &_user_params.midtone_offset, 0.005f);

      ImGui::ColorEdit3("Hightlight Colour", &_user_params.highlight_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Hightlight Offset", &_user_params.highlight_offset, 0.005f);
   }

   ImGui::Separator();

   if (ImGui::Button("Reset Settings")) {
      _user_params = Color_grading_params{};
   }

   ImGui::End();

   update_eval_params();
}

void Color_grading::bind_lut(int r_slot, int g_slot, int b_slot) noexcept
{
   if (std::exchange(_lut_dirty, false)) bake_lut();

   Ensures(_r_texture && _g_texture && _b_texture);

   setup_lut_samplers({r_slot, g_slot, b_slot});

   _device->SetTexture(r_slot, _r_texture.get());
   _device->SetTexture(g_slot, _g_texture.get());
   _device->SetTexture(b_slot, _b_texture.get());
}

void Color_grading::update_eval_params() noexcept
{
   _lut_dirty = true;

   _color_filter_exposure =
      _user_params.color_filter * glm::exp2(_user_params.exposure);
   _saturation = glm::max(_user_params.saturation, 0.0f);

   _contrast_strength = _user_params.contrast;

   _filmic_curve = filmic::color_grading_params_to_curve(_user_params);

   _srgb = _user_params.srgb;

   auto lift = _user_params.shadow_color;
   lift -= (lift.x + lift.y + lift.z) / 3.0f;

   auto gamma = _user_params.midtone_color;
   gamma -= (gamma.x + gamma.y + gamma.z) / 3.0f;

   auto gain = _user_params.highlight_color;
   gain -= (gain.x + gain.y + gain.z) / 3.0f;

   _lift_adjust = 0.0f + (lift + _user_params.shadow_offset);
   _gain_adjust = 1.0f + (gain + _user_params.highlight_offset);

   const auto mid_grey = 0.5f + (gamma + _user_params.midtone_offset);

   _inv_gamma_adjust = 1.0f / (glm::log(0.5f - _lift_adjust) /
                               (_gain_adjust - _lift_adjust) / glm::log(mid_grey));

   _max_table_value =
      eval_log_contrast_reverse(_filmic_curve.w, _contrast_epsilon,
                                _contrast_log_midpoint, _contrast_strength);

   _color_filter_exposure *= (1.0f / _max_table_value);
}

void Color_grading::bake_lut() noexcept
{
   std::vector<glm::uint8> r;
   r.resize(_curve_size);
   std::vector<glm::uint8> g;
   g.resize(_curve_size);
   std::vector<glm::uint8> b;
   b.resize(_curve_size);

   for (int i = 0; i < _curve_size; i++) {
      float t = float(i) / float(_curve_size - 1);

      t = (t * t) * _max_table_value;

      glm::vec3 rgb{t};
      rgb = eval_log_contrast(rgb, _contrast_epsilon, _contrast_log_midpoint,
                              _contrast_strength);
      rgb = filmic::eval(rgb, _filmic_curve);

      if (_srgb) rgb = linear_to_srgb(rgb);

      rgb = eval_lift_gamma_gain(rgb, _lift_adjust, _inv_gamma_adjust, _gain_adjust);

      r[i] = glm::packUnorm1x8(rgb.r);
      g[i] = glm::packUnorm1x8(rgb.g);
      b[i] = glm::packUnorm1x8(rgb.b);
   }

   create_lut(r, _r_texture);
   create_lut(g, _g_texture);
   create_lut(b, _b_texture);
}

void Color_grading::create_lut(const std::vector<glm::uint8>& texels,
                               Com_ptr<IDirect3DTexture9>& texture) noexcept
{
   Expects(_curve_size == static_cast<int>(texels.size()));

   if (!texture) {
      const auto usage = _use_dynamic_textures ? D3DUSAGE_DYNAMIC : 0;
      const auto pool = _use_dynamic_textures ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;

      const auto result =
         _device->CreateTexture(_curve_size, 1, 1, usage, D3DFMT_L8, pool,
                                texture.clear_and_assign(), nullptr);

      if (FAILED(result)) {
         log_and_terminate("Failed to create LUT for colour grading."s);
      }
   }

   upload_lut(texels, *texture);
}

void Color_grading::upload_lut(const std::vector<glm::uint8>& texels,
                               IDirect3DTexture9& texture) noexcept
{
   D3DLOCKED_RECT rect;

   texture.LockRect(0, &rect, nullptr, D3DLOCK_DISCARD);

   constexpr auto texel_size = sizeof(std::decay_t<decltype(texels)>::value_type);

   Ensures(static_cast<INT>(texels.size() * texel_size) == rect.Pitch);

   std::memcpy(rect.pBits, texels.data(), texels.size());

   texture.UnlockRect(0);
}

void Color_grading::setup_lut_samplers(std::initializer_list<int> indices) const noexcept
{
   for (const auto index : indices) {
      _device->SetSamplerState(index, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      _device->SetSamplerState(index, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      _device->SetSamplerState(index, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      _device->SetSamplerState(index, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      _device->SetSamplerState(index, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
      _device->SetSamplerState(index, D3DSAMP_SRGBTEXTURE, false);
   }
}

}
