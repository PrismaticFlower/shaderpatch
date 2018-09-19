
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"

struct Vs_textured_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
};

Texture2D<float4> flare_map : register(ps_3_0, s0);
SamplerState linear_clamp_sampler;

Vs_textured_output flare_textured_vs(Vertex_input input)
{
   Vs_textured_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();
   output.color = get_material_color(input.color()) * hdr_info.zzzw;
   output.texcoords = input.texcoords();

   return output;
}

struct Ps_textured_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
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
   float4 positionPS : SV_Position;
   float4 color : TEXCOORD;
};

Vs_untextured_output flare_untextured_vs(Vertex_input input)
{
   Vs_untextured_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();
   output.color = get_material_color(input.color()) * hdr_info.zzzw;

   return output;
}

float4 flare_untextured_ps(float4 color : TEXCOORD) : SV_Target0
{
   return color;
}

