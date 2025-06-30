
#include "color_utilities.hlsl"
#include "film_grain.hlsl"
#include "postprocess_common.hlsl"

// clang-format off

#pragma warning(disable : 3571)

const static bool bloom_thresholded = BLOOM_THRESHOLDED;
const static bool vignette = VIGNETTE_ACTIVE;
const static bool color_grading_active = COLOR_GRADING_ACTIVE;
const static bool convert_to_srgb = CONVERT_TO_SRGB;
const static bool dithering_active = DITHERING_ACTIVE;
const static bool output_cmaa2_luma = OUTPUT_CMAA2_LUMA;

void apply_bloom(float2 texcoords, inout float3 color)
{
   float3 bloom_color = bloom_tent9_upsample(bloom_texture, texcoords);
   
   if (bloom_use_dirt) {
      const float3 dirt = dirt_texture.Sample(linear_border_sampler, texcoords);

      bloom_color += (bloom_color * (dirt * bloom_dirt_scale));
   }

   bloom_color *= bloom_global_scale;

   if (bloom_thresholded) {
      color += (bloom_color * bloom_global_scale);
   }
   else {
      color = lerp(color, bloom_color, bloom_blend);
   }
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

struct Postprocessing_output
{
   float3 color : SV_Target0;

#  if OUTPUT_CMAA2_LUMA
      float luma : SV_Target1;
#  endif
};

struct Vs_output
{
   float2 texcoords : TEXCOORD;
   float2 positionSS : POSITION;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) {
      output.positionPS = float4(-1.f, -1.f, 0.0, 1.0);
      output.texcoords = float2(0.0, 1.0);
      output.positionSS = float2(0.0, 1.0) * resolution;
   }
   else if (id == 1) {
      output.positionPS = float4(-1.f, 3.f, 0.0, 1.0);
      output.texcoords = float2(0.0, -1.0);
      output.positionSS = float2(0.0, -1.0) * resolution;
   }
   else {
      output.positionPS = float4(3.f, -1.f, 0.0, 1.0);
      output.texcoords = float2(2.0, 1.0);
      output.positionSS = float2(2.0, 1.0) * resolution;
   }

   return output;
}

Postprocessing_output main_ps(float2 texcoords : TEXCOORD, float2 positionSS : POSITION)
{
   float3 color = scene_texture[(uint2)positionSS.xy].rgb;

   if (bloom) apply_bloom(texcoords, color);

   if (vignette) apply_vignette(texcoords, color);

   if (color_grading_active) apply_color_grading(color);

   if (convert_to_srgb) color = linear_to_srgb(color);

   if (film_grain) filmgrain::apply(texcoords, color);

   if (dithering_active) apply_dithering(color, (uint2)positionSS.xy);

   Postprocessing_output output;

   output.color = color;

#  if OUTPUT_CMAA2_LUMA
      output.luma = dot(sqrt(color), float3(0.299, 0.587, 0.114));
#  endif

   return output;
}

float3 stock_hdr_to_linear_ps(float2 texcoords : TEXCOORD, float2 positionSS : POSITION) : SV_Target0
{
   const float3 color_srgb = scene_texture[(uint2)positionSS.xy].rgb;
   const float3 color_linear = srgb_to_linear(color_srgb + color_srgb);

   return color_linear;
}
