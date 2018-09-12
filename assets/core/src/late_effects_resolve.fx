
#include "vertex_utilities.hlsl"

sampler2D backbuffer : register(s0);

float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD) : POSITION
{
   return float4(position, 0.0, 1.0);
}

float4 main_ps(float2 texcoord : TEXCOORD) : COLOR
{
   return tex2D(backbuffer, texcoord);
}