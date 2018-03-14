
// Force the vertex color to be used.
#ifndef USE_VERTEX_COLOR
#define USE_VERTEX_COLOR
#endif

#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 position : POSITION;
   float4 color : COLOR;
};

Vs_output rain_vs(Vs_input input)
{
   Vs_output output;
    
   output.position = transform::position_project(input.position);
   output.color = get_material_color(input.color);

   return output;
}

float4 rain_ps(float4 color : COLOR) : COLOR
{
   return color;
}