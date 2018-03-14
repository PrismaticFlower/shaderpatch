
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_textured_input
{
   float4 position : POSITION;
   float4 color : COLOR;
   float4 texcoord : TEXCOORD;
};

struct Vs_textured_output
{
   float4 position : POSITION;
   float4 color : COLOR;
   float2 texcoord : TEXCOORD;
};

struct Ps_textured_input
{
   float4 color : COLOR;
   float2 texcoord : TEXCOORD;
};

sampler2D diffuse_map;

Vs_textured_output flare_textured_vs(Vs_textured_input input)
{
   Vs_textured_output output;

   output.position = transform::position_project(input.position);
   output.color = get_material_color(input.color) * hdr_info.zzzw;
   output.texcoord = decompress_texcoords(input.texcoord);

   return output;
}

float4 flare_textured_ps(Ps_textured_input input) : COLOR
{
   float4 color;
   color.rgb = input.color.rgb;
   color.a = input.color.a * tex2D(diffuse_map, input.texcoord).a;

   return color;
}

struct Vs_untextured_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_untextured_output
{
   float4 position : POSITION;
   float4 color : COLOR;
};

Vs_untextured_output flare_untextured_vs(Vs_untextured_input input)
{
   Vs_untextured_output output;

   output.position = transform::position_project(input.position);
   output.color = get_material_color(input.color) * hdr_info.zzzw;

   return output;
}

float4 flare_untextured_ps(float4 color : COLOR) : COLOR
{
   return color;
}

