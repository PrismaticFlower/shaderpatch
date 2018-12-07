
#include "vertex_utilities.hlsl"

Texture2D backbuffer_texture : register(t0);

float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD) : POSITION
{
   return float4(position, 0.0, 1.0);
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return backbuffer_texture.SampleLevel(point_clamp_sampler, texcoords, 0);
}