
#include "color_utilities.hlsl"

Texture2D<float3> backbuffer_texture;
Texture2DMS<float3> backbuffer_texture_ms;

float4 main_vs(uint id : SV_VertexID) : SV_Position
{
   if (id == 0) return float4(-1.f, -1.f, 0.0, 1.0);
   else if (id == 1) return float4(-1.f, 3.f, 0.0, 1.0);
   else return float4(3.f, -1.f, 0.0, 1.0);
}

float4 main_ps(float4 positionSS : SV_Position) : SV_Target0
{
   return float4(backbuffer_texture[(uint2)positionSS], 1.0);
}

float4 main_ms_ps(float4 positionSS : SV_Position) : SV_Target0
{
   uint width, height, sample_count;

   backbuffer_texture_ms.GetDimensions(width, height, sample_count);

   const uint2 pos = (uint2)positionSS;
   float3 color;

   [loop] for (uint i = 0; i < sample_count; ++i) {
      color += backbuffer_texture_ms.sample[i][pos];
   }

   color *= rcp((float)sample_count);

   return float4(color, 1.0);
}

float4 main_linear_ps(float4 positionSS : SV_Position) : SV_Target0
{
   const float3 lin_color = backbuffer_texture[(uint2)positionSS];
   const float3 srgb_color = linear_to_srgb(lin_color);

   return float4(srgb_color, 1.0);
}

float4 main_linear_ms_ps(float4 positionSS : SV_Position) : SV_Target0
{
   uint width, height, sample_count;

   backbuffer_texture_ms.GetDimensions(width, height, sample_count);

   const uint2 pos = (uint2)positionSS;
   float3 lin_color;

   [loop] for (uint i = 0; i < sample_count; ++i) {
      lin_color += backbuffer_texture_ms.sample[i][pos];
   }

   lin_color *= rcp((float)sample_count);

   const float3 srgb_color = linear_to_srgb(lin_color);

   return float4(srgb_color, 1.0);
}