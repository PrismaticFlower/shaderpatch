
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "pixel_utilities.hlsl"

Texture2D<float3> scene_texture : register(ps_3_0, s0);

SamplerState linear_clamp_sampler;

float4 position_offset : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4 texcoords_offsets : register(vs, c[CUSTOM_CONST_MIN + 2]);

struct Vs_input
{
   float2 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_screenspace_output
{
   float4 position : SV_Position;
   float2 texcoords : TEXCOORD;
};

Vs_screenspace_output screenspace_vs(Vs_input input)
{
   Vs_screenspace_output output;

   output.texcoords = input.texcoords;

   float2 position = input.position;

   position = position + -position_offset.zw;
   position *= 2.0;
   position = position * float2(1.0, -1.0) + float2(-1.0, 1.0);

   output.position = float4(position, 0.5, 1.0);

   return output;
}

struct Vs_bloomfilter_output
{
   float4 position : SV_Position;
   float2 texcoords[4] : TEXCOORD0;
};

Vs_bloomfilter_output bloomfilter_vs(Vs_input input)
{
   Vs_bloomfilter_output output;

   float2 texcoords = input.texcoords;

   output.texcoords[0] = texcoords * texcoords_offsets.xy + texcoords_offsets.zw;

   output.texcoords[1].x = texcoords.x * texcoords_offsets.x + -texcoords_offsets.z;
   output.texcoords[1].y = texcoords.y * texcoords_offsets.y + texcoords_offsets.w;

   output.texcoords[2].x = texcoords.x * texcoords_offsets.x + texcoords_offsets.z;
   output.texcoords[2].y = texcoords.y * texcoords_offsets.y + -texcoords_offsets.w;

   output.texcoords[3] = texcoords * texcoords_offsets.xy + -texcoords_offsets.zw;

   float2 position = input.position;

   position = position + -position_offset.zw;
   position *= 2.0;
   position = position * float2(1.0, -1.0) + float2(-1.0, 1.0);

   output.position = float4(position, 0.5, 1.0);

   return output;
}

float4 ps_constants[3] : register(ps, c[0]);

const static float3 luma_weights = ps_constants[0].rgb;
const static float color_bias = ps_constants[0].a;
const static float luma_bias = 0.45;
const static float3 screenspace_tint = ps_constants[2].rgb;
const static float screenspace_alpha = ps_constants[2].a;
const static float blur_weight = ps_constants[1].a;

float4 glowthreshold_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 color = scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);

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

float4 bloomfilter_ps(float2 texcoords[4] : TEXCOORD0) : SV_Target0
{
   float3 color = 0.0;

   [unroll] for (int i = 0; i < 4; ++i) {
      color += scene_texture.SampleLevel(linear_clamp_sampler, texcoords[i], 0) * blur_weight;
   }

   return float4(color, 1.0);
}

float4 screenspace_ex_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 color = gaussian_sample(scene_texture, linear_clamp_sampler, texcoords, rt_resolution.zw).rgb;

   return float4(color * screenspace_tint, screenspace_alpha);
}

float4 bloomfilter_ex_ps(float2 texcoords[4] : TEXCOORD0) : SV_Target0
{
   float3 color = 0.0;

   [unroll] for (int i = 0; i < 4; ++i) {
      color += gaussian_sample(scene_texture, linear_clamp_sampler, texcoords[i], rt_resolution.zw) * blur_weight;
   }

   return float4(color, 1.0);
}
