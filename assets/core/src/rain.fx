#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"

struct Vs_output
{
   float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Vs_output rain_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();
   output.color = get_material_color(input.color());

   return output;
}

float4 rain_ps(float4 color : COLOR) : SV_Target0
{
   return color;
}

[earlydepthstencil]
void oit_rain_ps(float4 color : COLOR, float4 positionSS : SV_Position, uint coverage : SV_Coverage) 
{
   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, coverage, color);
}
