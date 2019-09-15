// clang-format off

#include "scope_blur_common.hlsl"

Texture2D<float4> src_texture;

SamplerState samp;

float4 blur_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return scope_blur(texcoords, 0, src_texture, samp);
}

float4 overlay_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return src_texture.SampleLevel(samp, texcoords, 0);
}
