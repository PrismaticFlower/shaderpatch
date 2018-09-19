
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"

struct Vs_output
{
   float4 positionPS : SV_Position;
   float4 color : TEXCOORD;
};

Vs_output rain_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();
   output.color = get_material_color(input.color());

   return output;
}

float4 rain_ps(float4 color : TEXCOORD) : SV_Target0
{
   return color;
}