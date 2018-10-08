
#include "fullscreen_tri_vs.hlsl"

Texture2D<float3> backbuffer_texture : register(ps_3_0, s0);
SamplerState point_clamp_sampler;

float alpha : register(c60);

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return float4(backbuffer_texture.SampleLevel(point_clamp_sampler, texcoords, 0), alpha);
}