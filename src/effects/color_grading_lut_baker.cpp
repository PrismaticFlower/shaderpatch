
#include "color_grading_lut_baker.hpp"
#include "../logger.hpp"
#include "utility.hpp"

using namespace std::literals;

namespace sp::effects {

Color_grading_lut_baker::Color_grading_lut_baker(Com_ptr<ID3D11Device1> device,
                                                 shader::Database& shaders) noexcept
   : _device{std::move(device)},
     _cs_main_filmic{
        shaders.compute("color grading lut baker"sv).entrypoint("main filmic"sv)},
     _cs_main_aces{
        shaders.compute("color grading lut baker"sv).entrypoint("main aces"sv)},
     _cs_main_heji2015{
        shaders.compute("color grading lut baker"sv).entrypoint("main heji2015"sv)},
     _cs_main_reinhard{
        shaders.compute("color grading lut baker"sv).entrypoint("main reinhard"sv)},
     _cs_main{shaders.compute("color grading lut baker"sv).entrypoint("main"sv)}
{
}

auto Color_grading_lut_baker::srv() noexcept -> ID3D11ShaderResourceView*
{
   return _srv.get();
}

void Color_grading_lut_baker::bake_color_grading_lut(ID3D11DeviceContext1& dc,
                                                     const Color_grading_params& params) noexcept
{
   update_cb(dc, params);

   dc.CSSetShader(pick_shader(params.tonemapper), nullptr, 0);

   auto* const uav = _uav.get();
   dc.CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

   auto* const cb = _cb.get();
   dc.CSSetConstantBuffers(0, 1, &cb);

   const auto dispatch_dim = safe_max(lut_dimension / group_size, 1u);
   dc.Dispatch(dispatch_dim, dispatch_dim, dispatch_dim);

   ID3D11UnorderedAccessView* const null_uav = nullptr;
   dc.CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
}

void Color_grading_lut_baker::update_cb(ID3D11DeviceContext1& dc,
                                        const Color_grading_params& params) noexcept
{
   const Color_grading_lut_cb cb{params};

   core::update_dynamic_buffer(dc, *_cb, cb);
}

auto Color_grading_lut_baker::pick_shader(const Tonemapper tonemapper) noexcept
   -> ID3D11ComputeShader*
{
   switch (tonemapper) {
   case Tonemapper::filmic:
      return _cs_main_filmic.get();
   case Tonemapper::aces_fitted:
      return _cs_main_aces.get();
   case Tonemapper::filmic_heji2015:
      return _cs_main_heji2015.get();
   case Tonemapper::reinhard:
      return _cs_main_reinhard.get();
   case Tonemapper::none:
      return _cs_main.get();
   }

   std::terminate();
}

Color_grading_lut_baker::Color_grading_lut_cb::Color_grading_lut_cb(
   const Color_grading_params& params) noexcept
   : filmic_curve{filmic::color_grading_params_to_curve(params)}
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

   heji_whitepoint = params.filmic_heji_whitepoint;
}

Color_grading_lut_baker::Color_grading_lut_cb::Filmic_curve::Filmic_curve(
   const filmic::Curve& curve) noexcept
{
   w = curve.w;
   inv_w = curve.inv_w;
   x0 = curve.x0;
   y0 = curve.y0;
   x1 = curve.x1;
   y1 = curve.y1;

   for (auto i = 0; i < segments.size(); ++i) {
      segments[i].offset_x = curve.segments[i].offset_x;
      segments[i].offset_y = curve.segments[i].offset_y;
      segments[i].scale_x = curve.segments[i].scale_x;
      segments[i].scale_y = curve.segments[i].scale_y;
      segments[i].ln_a = curve.segments[i].ln_a;
      segments[i].b = curve.segments[i].b;
   }
}

}
