
#include "color_utilities.hlsl"

// clang-format off

cbuffer ResolveConstants : register(b0)
{
   int2 input_range;
   float exposure;
}

#if RESOLVE_SAMPLE_COUNT == 2
   static const float2 sample_offsets[2] = {
      float2(0.25, 0.25),
      float2(-0.25, -0.25)
   };
#elif RESOLVE_SAMPLE_COUNT == 4
   static const float2 sample_offsets[4] = {
      float2(-0.125, -0.375),
      float2(0.375, 0.125),
      float2(-0.125, 0.375),
      float2(0.125, 0.375)
   };
#elif RESOLVE_SAMPLE_COUNT == 8
   static const float2 sample_offsets[8] = {
      float2(0.0625, -0.1875),
      float2(-0.0625,  0.1875),
      float2(0.3125,  0.0625),
      float2(-0.1875, -0.3125),
      float2(-0.3125,  0.3125),
      float2(-0.4375, -0.0625),
      float2(0.1875,  0.4375),
      float2(0.4375, -0.4375)
   };
#endif

static const uint sample_count = RESOLVE_SAMPLE_COUNT;
static const int resolve_radius = 1;
static const float filter_radius = (float)resolve_radius;

Texture2DMS<float3> input;

float filter_bspline(float x_unscaled)
{
   const float B = 1.0;
   const float C = 0.0;

   const float x = x_unscaled * 2.0;
   const float x2 = x * x;
   const float x3 = x * x * x;

   if (x < 1.0)
      return ((12.0 - 9.0 * B - 6.0 * C) * x3 + (-18.0 + 12.0 * B + 6.0 * C) * x2 + (6.0 - 2.0 * B)) / 6.0;
   else if (x <= 2.0)
      return ((-B - 6.0 * C) * x3 + (6.0 * B + 30.0 * C) * x2 + (-12.0 * B - 48.0 * C) * x + (8.0 * B + 24.0 * C)) / 6.0;

   return 0.0;
}

float4 main_hdr_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   const int2 pos = (int2)positionSS.xy;
   float3 color = 0.0;
   float total_weight = 0.0;

   [unroll]
   for (int y = -resolve_radius; y <= resolve_radius; ++y)
   {
      [unroll]
      for (int x = -resolve_radius; x <= resolve_radius; ++x) {
         const float2 pixel_offset = float2(x, y);
         const uint2 pixel_pos = clamp(pos + int2(x, y), 0, input_range);

         [unroll] for (uint samp_index = 0; samp_index < sample_count; ++samp_index) {
            const float2 sample_offset = sample_offsets[samp_index];
            const float2 sample_distance = abs(sample_offset + pixel_offset) / filter_radius;

            if (!all(sample_distance <= 1.0)) continue;

            const float3 sample = max(input.sample[samp_index][pixel_pos], 0.0);

            float weight = filter_bspline(sample_distance.x) * filter_bspline(sample_distance.y);

            const float luminance = dot(sample, float3(0.2126, 0.7152, 0.0722)) * exposure;
            
            weight *= (1.0 / (1.0 + luminance));

            color += sample * weight;
            total_weight += weight;
         }
      }
   }

   color /= max(total_weight, 1e-5);

   return float4(color, 1.0);
}

float4 main_stock_hdr_ps(float2 texcoords : TEXCOORD, float4 positionSS : SV_Position) : SV_Target0
{
   const int2 pos = (int2)positionSS.xy;
   float3 color = 0.0;
   float total_weight = 0.0;

   [unroll]
   for (int y = -resolve_radius; y <= resolve_radius; ++y) {
   [unroll]
      for (int x = -resolve_radius; x <= resolve_radius; ++x) {
         const float2 pixel_offset = float2(x, y);
         const uint2 pixel_pos = clamp(pos + int2(x, y), 0, input_range);

         [unroll] for (uint samp_index = 0; samp_index < sample_count; ++samp_index) {
            const float2 sample_offset = sample_offsets[samp_index];
            const float2 sample_distance = abs(sample_offset + pixel_offset) / filter_radius;

            if (!all(sample_distance <= 1.0)) continue;

            const float3 sample_srgb = input.sample[samp_index][pixel_pos];
            const float3 sample_linear = srgb_to_linear(sample_srgb + sample_srgb);

            const float weight = filter_bspline(sample_distance.x) * filter_bspline(sample_distance.y);

            color += sample_linear * weight;
            total_weight += weight;
         }
      }
   }

   color /= max(total_weight, 1e-5);

   return float4(color, 1.0);
}