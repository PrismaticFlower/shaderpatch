#include "generic_vertex_input.hlsl"

float4 discard_vs(Vertex_input input) : SV_Position
{
   return 0.0;
}

float4 discard_ps() : SV_Target0
{
   discard;

   return float4(0.0, 0.0, 0.0, 1.0);
}
