
#include "vertex_utilities.hlsl"

float4 shadowquad_vs(float4 position : POSITION) : POSITION
{
   position = decompress_position(position);
    
   return float4(position.xy, 0.5, 1.0);

}

float4 shadowquad_ps(uniform float4 intensity : register(ps, c[0])) : COLOR
{
   return intensity.aaaa;
}