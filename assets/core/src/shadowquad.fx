
#include "constants_list.hlsl"

const static float intensity = ps_custom_constants[0].a;

float4 shadowquad_vs(float3 position : POSITION) : SV_Position
{
   return float4(position.xy, 0.5, 1.0);

}

float4 shadowquad_ps() : SV_Target0
{
   return intensity;
}