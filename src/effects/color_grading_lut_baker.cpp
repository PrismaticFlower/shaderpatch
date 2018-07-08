
#include "color_grading_lut_baker.hpp"

#include <vector>

#include <glm/gtc/packing.hpp>

namespace sp::effects {

using namespace std::literals;

namespace {

const auto log_contrast_midpoint = glm::log2(.18f);
constexpr auto log_contrast_epsilon = 1e-5f;
constexpr auto lut_size = 256;

constexpr Sampler_info lut_sampler_info = {D3DTADDRESS_CLAMP,
                                           D3DTADDRESS_CLAMP,
                                           D3DTADDRESS_CLAMP,
                                           D3DTEXF_LINEAR,
                                           D3DTEXF_LINEAR,
                                           D3DTEXF_NONE,
                                           0.0f,
                                           false};

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

struct Eval_params {
   float contrast;

   Filmic_curve filmic;

   glm::vec3 lift_adjust;
   glm::vec3 inv_gamma_adjust;
   glm::vec3 gain_adjust;

   float max_table_value;
};

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
}

glm::vec3 eval_log_contrast(glm::vec3 v, float contrast)
{
   glm::vec3 log_v = glm::log2(v + log_contrast_epsilon);
   glm::vec3 adj_v = log_contrast_midpoint + (log_v - log_contrast_midpoint) * contrast;

   return glm::max(glm::vec3{0.0f}, glm::exp2(adj_v) - log_contrast_epsilon);
}

float eval_log_contrast_reverse(float v, float contrast)
{
   float log_v = glm::log2(v + log_contrast_epsilon);
   float adj_v = (log_v - log_contrast_midpoint) / contrast + log_contrast_midpoint;

   return glm::max(0.0f, glm::exp2(adj_v) - log_contrast_epsilon);
}

glm::vec3 eval_lift_gamma_gain(glm::vec3 v, glm::vec3 lift_adjust,
                               glm::vec3 inv_gamma_adjust, glm::vec3 gain_adjust)
{
   v = glm::clamp((glm::pow(v, inv_gamma_adjust)), 0.0f, 1.0f);

   return gain_adjust * v + lift_adjust * (1.0f - v);
}

auto get_eval_params(const Color_grading_params& params) noexcept -> Eval_params
{
   Eval_params eval;

   eval.contrast = params.contrast;

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

   eval.filmic = filmic::color_grading_params_to_curve(params);

   eval.max_table_value = eval_log_contrast_reverse(eval.filmic.w, eval.contrast);

   return eval;
}

auto create_sp_lut(IDirect3DDevice9& device, IDirect3DTexture9& d3d_texture) -> Texture
{
   Com_ptr<IDirect3DBaseTexture9> base_texture;
   d3d_texture.QueryInterface(IID_IDirect3DBaseTexture9,
                              base_texture.void_clear_and_assign());

   device.AddRef();

   return Texture{Com_ptr{&device}, std::move(base_texture), lut_sampler_info};
}

void upload_d3d_lut(const std::vector<glm::u16>& texels, IDirect3DTexture9& texture) noexcept
{
   D3DLOCKED_RECT rect;

   texture.LockRect(0, &rect, nullptr, D3DLOCK_DISCARD);

   constexpr auto texel_size = sizeof(std::decay_t<decltype(texels)>::value_type);

   Ensures(static_cast<INT>(texels.size() * texel_size) == rect.Pitch);

   std::memcpy(rect.pBits, texels.data(), texels.size() * texel_size);

   texture.UnlockRect(0);
}

auto create_lut(IDirect3DDevice9& device, const std::vector<glm::u16>& texels) noexcept
   -> Texture
{
   Expects(lut_size == static_cast<int>(texels.size()));

   Com_ptr<IDirect3DTexture9> d3d_texture;

   const auto result =
      device.CreateTexture(lut_size, 1, 1, 0, D3DFMT_L16, D3DPOOL_MANAGED,
                           d3d_texture.clear_and_assign(), nullptr);

   if (FAILED(result)) {
      log_and_terminate("Failed to create LUT for colour grading."s);
   }

   upload_d3d_lut(texels, *d3d_texture);

   return create_sp_lut(device, *d3d_texture);
}

}

glm::vec3 get_lut_exposure_color_filter(const Color_grading_params& params) noexcept
{
   const auto color_filter_exposure =
      params.color_filter * glm::exp2(params.exposure) * params.brightness;

   const auto filmic_curve = filmic::color_grading_params_to_curve(params);

   const auto max_table_value =
      eval_log_contrast_reverse(filmic_curve.w, params.contrast);

   return color_filter_exposure * (1.0f / max_table_value);
}

auto bake_color_grading_luts(IDirect3DDevice9& device,
                             const Color_grading_params& params) noexcept
   -> std::array<Texture, 3>
{
   const auto eval = get_eval_params(params);

   std::vector<glm::u16> r;
   r.resize(lut_size);
   std::vector<glm::u16> g;
   g.resize(lut_size);
   std::vector<glm::u16> b;
   b.resize(lut_size);

   for (int i = 0; i < lut_size; ++i) {
      float t = float(i) / float(lut_size - 1);

      t = (t * t) * eval.max_table_value;

      glm::vec3 rgb{t};
      rgb = eval_log_contrast(rgb, eval.contrast);
      rgb = eval_lift_gamma_gain(rgb, eval.lift_adjust, eval.inv_gamma_adjust,
                                 eval.gain_adjust);
      rgb = filmic::eval(rgb, eval.filmic);

      r[i] = glm::packUnorm1x16(rgb.r);
      g[i] = glm::packUnorm1x16(rgb.g);
      b[i] = glm::packUnorm1x16(rgb.b);
   }

   return {create_lut(device, r), create_lut(device, g), create_lut(device, b)};
}

}
