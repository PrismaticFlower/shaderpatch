
#include "adaptive_oit.hlsl"

#pragma warning(disable : 4000)

float4 main_vs(uint id : SV_VertexID) : SV_Position
{
   if (id == 0)
      return float4(-1.f, -1.f, 0.0, 1.0);
   else if (id == 1)
      return float4(-1.f, 3.f, 0.0, 1.0);

   return float4(3.f, -1.f, 0.0, 1.0);
}

float4 main_ps(float4 positionSS : SV_POSITION) : SV_Target
{
   return aoit::resolve_pixel((uint2)positionSS.xy);
}
