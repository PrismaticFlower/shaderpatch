
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"

float4 prereflection_vs(Vertex_input input) : SV_Position
{
   Transformer transformer = create_transformer(input);

   return transformer.positionPS();
}

float4 prereflection_fake_stencil_vs(Vertex_input input) : SV_Position
{
   Transformer transformer = create_transformer(input);

   return transformer.positionPS().xyww;
}

float4 prereflection_ps() : SV_Target0
{
   return 0.0;
}