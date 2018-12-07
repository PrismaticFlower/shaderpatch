
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_sampler_states.hlsl"

struct Vs_textured_output
{
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Texture2D<float4> flare_map : register(t0);

Vs_textured_output flare_textured_vs(Vertex_input input)
{
   Vs_textured_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();

   float4 material_color = get_material_color(input.color());
   material_color.rgb *= lighting_scale;

   output.color = material_color;
   output.texcoords = input.texcoords();

   return output;
}

struct Ps_textured_input
{
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
};

float4 flare_textured_ps(Ps_textured_input input) : SV_Target0
{
   float4 color;
   color.rgb = input.color.rgb;
   color.a = input.color.a * saturate(flare_map.Sample(linear_clamp_sampler, input.texcoords).a);

   return color;
}

struct Vs_untextured_output
{
   float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Vs_untextured_output flare_untextured_vs(Vertex_input input)
{
   Vs_untextured_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();

   float4 material_color = get_material_color(input.color());
   material_color.rgb *= lighting_scale;

   output.color = material_color;

   return output;
}

float4 flare_untextured_ps(float4 color : COLOR) : SV_Target0
{
   return color;
}

