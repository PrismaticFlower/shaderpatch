
#include "constants_list.hlsl"

const static float intensity = ps_custom_constants[0].a;

float4 shadowquad_vs(uint id : SV_VertexID) : SV_Position
{
   if (id == 0) return float4(-1.f, -1.f, 0.0, 1.0);
   else if (id == 1) return float4(-1.f, 3.f, 0.0, 1.0);
   else return float4(3.f, -1.f, 0.0, 1.0);

}

float4 shadowquad_ps() : SV_Target0
{
   return intensity;
}