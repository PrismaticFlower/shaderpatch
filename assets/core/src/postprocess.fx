
#include "aa_quality_levels.hlsl"
#include "fullscreen_tri_vs.hlsl"
#include "pixel_utilities.hlsl"
#include "postprocess_common.hlsl"

#include "FXAA3_11.h"

#pragma warning(disable : 3571)

float3 combine_bloom(float2 texcoords)
{
   float3 color = bloom_tent9_upsample(bloom_sampler, texcoords);
   
   if (bloom_use_dirt) {
      color += color * (tex2D(dirt_sampler, texcoords).rgb * bloom_dirt_scale);
   }
   
   return color * bloom_global_scale;
}

float sample_lut(sampler2D lut, float v)
{
   return tex2D(lut, float2(sqrt(v) + 1.0 / 256.0, 0.0)).x;
}

float3 apply_color_grading(float3 color)
{
   color *= exposure_color_filter;

   float grey = dot(saturation_weights, color);
   color = grey + (saturation * (color - grey));

   color.r = sample_lut(red_lut, color.r);
   color.g = sample_lut(green_lut, color.g);
   color.b = sample_lut(blue_lut, color.b);

   return color;
}

float fxaa_luma(float3 color)
{
   return sqrt(dot(color, fxaa_luma_weights));
}

float4 postprocess_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 color = tex2D(scene_sampler, texcoords).rgb;

   if (stock_hdr) color = pow(color + color, 2.2);

   if (bloom) color += combine_bloom(texcoords);

   color = apply_color_grading(color);

   return float4(color, fxaa_luma(color));
}

float3 apply_dithering(float3 color, float2 position)
{
   float3 blue_noise = tex2Dlod(blue_noise_sampler, (position / 64.0) + randomness.xy, 0).rgb;
   blue_noise = blue_noise * 2.0 - 1.0;
   blue_noise = sign(blue_noise) * (1.0 - sqrt(1.0 - abs(blue_noise)));

   return color + blue_noise / 255.0;
}

float4 postprocess_finalize_ps(float2 texcoords : TEXCOORD, float2 position : VPOS) : COLOR
{
   float4 color = FxaaPixelShader(texcoords,
                                  0.0,
                                  scene_sampler,
                                  scene_sampler,
                                  scene_sampler,
                                  scene_pixel_size,
                                  0.0,
                                  0.0,
                                  0.0,
                                  fxaa_quality_subpix,
                                  fxaa_quality_edge_threshold,
                                  fxaa_quality_edge_threshold_min,
                                  0.0,
                                  0.0,
                                  0.0,
                                  0.0);

   color.rgb = apply_dithering(color.rgb, position);

   return color;
}