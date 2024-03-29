
#include "color_utilities.hlsl"

Texture2D<float3> backbuffer_texture;
Texture2DMS<float3> backbuffer_texture_ms;
Texture2D<float3> blue_noise_texture : register(t1);

const static uint sample_count = SAMPLE_COUNT;
const static float inv_sample_count = 1.0 / sample_count;

cbuffer Constants : register(b0) {
   float2 resolution;
   uint2 randomness;
};

float3 apply_dithering(const float3 color, const uint2 position)
{
   float3 blue_noise = blue_noise_texture[(position + randomness) & 0x3f];
   blue_noise = blue_noise * 2.0 - 1.0;
   blue_noise = sign(blue_noise) * (1.0 - sqrt(1.0 - abs(blue_noise)));

   return color + (blue_noise / 255.0);
}

struct Vs_output
{
   float2 positionSS : POSITION;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) {
      output.positionPS = float4(-1.f, -1.f, 0.0, 1.0);
      output.positionSS = float2(0.0, 1.0) * resolution;
   }
   else if (id == 1) {
      output.positionPS = float4(-1.f, 3.f, 0.0, 1.0);
      output.positionSS = float2(0.0, -1.0) * resolution;
   }
   else {
      output.positionPS = float4(3.f, -1.f, 0.0, 1.0);
      output.positionSS = float2(2.0, 1.0) * resolution;
   }

   return output;
}

float4 main_ps(float2 positionSS : POSITION) : SV_Target0
{
   const float3 color = backbuffer_texture[(uint2)positionSS];
   const float3 dithered_color = apply_dithering(color, (uint2)positionSS);

   return float4(dithered_color, 1.0);
}

float4 main_ms_ps(float2 positionSS : POSITION) : SV_Target0
{
   const uint2 pos = (uint2)positionSS;
   float3 color = 0.0;

   [unroll] for (uint i = 0; i < sample_count; ++i) {
      color += srgb_to_linear(backbuffer_texture_ms.sample[i][pos]);
   }

   color *= inv_sample_count;

   const float3 srgb_color = linear_to_srgb(color);
   const float3 dithered_srgb_color = apply_dithering(srgb_color, (uint2)positionSS);

   return float4(dithered_srgb_color, 1.0);
}

float4 main_linear_ps(float2 positionSS : POSITION) : SV_Target0
{
   const float3 color = backbuffer_texture[(uint2)positionSS];
   const float3 srgb_color = linear_to_srgb(color);
   const float3 dithered_srgb_color = apply_dithering(srgb_color, (uint2)positionSS);

   return float4(dithered_srgb_color, 1.0);
}

float4 main_linear_ms_ps(float2 positionSS : POSITION) : SV_Target0
{
   const uint2 pos = (uint2)positionSS;
   float3 color = 0.0;

   [unroll] for (uint i = 0; i < sample_count; ++i) {
      color += backbuffer_texture_ms.sample[i][pos];
   }

   color *= inv_sample_count;

   const float3 srgb_color = linear_to_srgb(color);
   const float3 dithered_srgb_color = apply_dithering(srgb_color, (uint2)positionSS);

   return float4(dithered_srgb_color, 1.0);
}