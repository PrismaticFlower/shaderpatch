#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"

// clang-format off

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

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;

   output.color = get_material_color(input.color());
   output.color.rgb *= lighting_scale;
   output.color.a *= calculate_near_fade_transparent(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

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

[earlydepthstencil]
void oit_lightbeam_ps(Ps_input input, float4 positionSS : SV_Position)
{
   const float4 color = lightbeam_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color);
}