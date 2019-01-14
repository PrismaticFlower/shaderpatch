
#include "fullscreen_tri_vs.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float3> backbuffer_texture : register(t0);

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return float4(pow(saturate(backbuffer_texture.SampleLevel(linear_clamp_sampler, texcoords, 0)), 2.2), 1.0);
}