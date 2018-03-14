
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

float4 prereflection_vs(float4 position : POSITION) : POSITION
{
   return transform::position_project(position);
}

float4 prereflection_fake_stencil_vs(float4 position : POSITION) : POSITION
{
   position = transform::position_project(position);

   return position.xyww;
}

float4 prereflection_ps() : COLOR
{
   return float4(0.0, 0.0, 0.0, 0.0);
}