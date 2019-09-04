#include "color_grading.hlsl"
#include "color_utilities.hlsl"
#include "tonemappers.hlsl"

// clang-format off

struct Params
{
   float3 color_filter;
   float contrast;
   float3 hsv_adjust;
   float saturation;

   float3 channel_mix_red;
   float3 channel_mix_green;
   float3 channel_mix_blue;

   float3 lift_adjust;
   float3 inv_gamma_adjust;
   float3 gain_adjust;

   float heji_whitepoint;
   Filmic_curve filmic_curve;
};

cbuffer ParamsConstants
{
   Params params;
};

const static uint tonemapper_none = 0;
const static uint tonemapper_filmic = 1;
const static uint tonemapper_aces_fitted = 2;
const static uint tonemapper_filmic_heji2015 = 3;
const static uint tonemapper_reinhard = 4;

const static uint tonemapper = CG_TONEMAPPER;
const static float lut_length = CG_LUT_LENGTH;

RWTexture3D<float4> lut : register(u0);

float3 apply_tonemapper(float3 color)
{
   switch (tonemapper) {
      case tonemapper_none:
         return color;
      case tonemapper_filmic:
         return eval_filmic(color, params.filmic_curve);
      case tonemapper_aces_fitted:
         return eval_aces_srgb_fitted(color);
      case tonemapper_filmic_heji2015:
         return eval_filmic_hejl2015(color, params.heji_whitepoint);
      case tonemapper_reinhard:
         return eval_reinhard(color);
   }
}

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID) {
   float3 color = (float3)id / (lut_length - 1.0);

   color = logc_to_linear(color);

   color = color * params.color_filter;
   color = apply_basic_saturation(color, params.saturation);
   color = apply_hsv_adjust(color, params.hsv_adjust);
   color = apply_channel_mixing(color, params.channel_mix_red, params.channel_mix_green, params.channel_mix_blue);
   color = apply_log_contrast(color, params.contrast);
   color = apply_lift_gamma_gain(color, params.lift_adjust, params.inv_gamma_adjust, params.gain_adjust);
   color = apply_tonemapper(color);

   lut[id] = float4(linear_to_srgb(color), 1.0);
}
