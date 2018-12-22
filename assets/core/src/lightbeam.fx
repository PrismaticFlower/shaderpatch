#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 color : COLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_output lightbeam_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);
   near_fade = saturate(near_fade);
   near_fade *= near_fade;

   float4 material_color = get_material_color(input.color());

   output.color.rgb = material_color.rgb * lighting_scale;
   output.color.a = saturate(material_color.a * near_fade);

   return output;
}

struct Ps_input
{
   float4 color : COLOR;
   float fog : FOG;
};

float4 lightbeam_ps(Ps_input input) : SV_Target0
{
   float4 color = input.color;

   color.rgb = apply_fog(color.rgb, input.fog);

   return color;
}