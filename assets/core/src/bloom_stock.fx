#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float3> scene_texture : register(t0);

const static float4 position_offset = custom_constants[1];
const static float4 texcoords_offsets = custom_constants[2];

struct Vs_input
{
   float2 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_screenspace_output
{
   float2 texcoords : TEXCOORD;
   float4 position : SV_Position;
};

Vs_screenspace_output screenspace_vs(Vs_input input)
{
   Vs_screenspace_output output;

   output.texcoords = input.texcoords;

   const float2 position = (2.0 * input.position) * float2(1.0, -1.0) + float2(-1.0, 1.0);

   output.position = float4(position, 0.5, 1.0);

   return output;
}

const static float3 luma_weights = float3(0.300, 0.590, 0.110);
const static float color_bias = ps_custom_constants[0].a;
const static float luma_bias = 0.45;
const static float3 screenspace_tint = ps_custom_constants[2].rgb;
const static float screenspace_alpha = ps_custom_constants[2].a;
const static float blur_weight = ps_custom_constants[1].a;

float4 glowthreshold_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const float3 color = scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);

   float luma = dot(saturate(color - color_bias), luma_weights) + luma_bias;

   if (luma > 0.5) return float4(color, luma);

   return float4(0.0.xxx, luma);
}

float4 luminance_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return dot(scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0), luma_weights);
}

float4 screenspace_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 color = scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);

   return float4(color * screenspace_tint, screenspace_alpha);
}

float4 bloomfilter_ps(float2 texcoords : TEXCOORD0) : SV_Target0
{
   const float2 offsets[16] = {
      {-2, -2}, {-1, -2}, {1, -2}, {2, -2},
      {-2, -1}, {-1, -1}, {1, -1}, {2, -1},
      {-2,  1}, {-1,  1}, {1,  1}, {2,  1},
      {-2,  2}, {-1,  2}, {1,  2}, {2,  2}};

   const float weights[16] = {
      0.015625, 0.046875, 0.046875, 0.015625,
      0.046875, 0.140625, 0.140625, 0.046875,
      0.046875, 0.140625, 0.140625, 0.046875,
      0.015625, 0.046875, 0.046875, 0.015625};

   float3 color = 0.0;

   [unroll] for (int i = 0; i < 16; ++i) {
      float2 uv = texcoords;
      uv += offsets[i] * rt_resolution.zw * 1.5;

      color += scene_texture.SampleLevel(linear_clamp_sampler, uv, 0) * weights[i];
   }

   const float filter_scale = blur_weight * 4.0;

   return float4(color * filter_scale, 1.0);
}

