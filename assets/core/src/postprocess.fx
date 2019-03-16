
#include "film_grain.hlsl"
#include "fullscreen_tri_vs.hlsl"
#include "pixel_utilities.hlsl"
#include "color_utilities.hlsl"

#pragma warning(disable : 3571)

void apply_bloom(float2 texcoords, inout float3 color)
{
   float3 bloom_color = bloom_tent9_upsample(bloom_texture, texcoords);
   
   if (bloom_use_dirt) {
      const float3 dirt = dirt_texture.Sample(linear_clamp_sampler, texcoords);

      bloom_color += (bloom_color * (dirt * bloom_dirt_scale));
   }
   
   color += (bloom_color * bloom_global_scale);
}

void apply_vignette(float2 texcoords, inout float3 color)
{
   float dist = distance(texcoords, float2(0.5, 0.5));

   color *= smoothstep(vignette_end, vignette_start, dist);
}

float3 sample_color_grading_lut(float3 color)
{
   const float3 scale = (color_grading_lut_size - 1.0f) / color_grading_lut_size;
   const float3 offset = (1.0f / color_grading_lut_size) / 2.0f;

   const float a = 5.555556;
   const float b = 0.047996;
   const float c = 0.244161;
   const float d = 0.386036;

   color = c * log10(a * color + b) + d;

   return color_grading_lut.SampleLevel(linear_clamp_sampler, color * scale + offset, 0);
}

void apply_color_grading(inout float3 color)
{
   color *= exposure;
   color = sample_color_grading_lut(color);
}

void apply_dithering(inout float3 color, uint2 position)
{
   float3 blue_noise = blue_noise_texture[(position + randomness_uint.xy) & 0x3f];
   blue_noise = blue_noise * 2.0 - 1.0;
   blue_noise = sign(blue_noise) * (1.0 - sqrt(1.0 - abs(blue_noise)));

   color += (blue_noise / 255.0);
}

float4 main_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   float3 color = scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0).rgb;

   if (bloom) apply_bloom(texcoords, color);

   if (vignette) apply_vignette(texcoords, color);

   apply_color_grading(color);

   float3 srgb_color = linear_to_srgb(color);

   if (film_grain) filmgrain::apply(texcoords, srgb_color);

   apply_dithering(srgb_color, (uint2)positionSS.xy);

   return float4(srgb_color, 1.0);
}

float3 stock_hdr_to_linear_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   const float3 color_srgb = scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0).rgb;
   const float3 color_linear = srgb_to_linear(color_srgb + color_srgb);

   return color_linear;
}
