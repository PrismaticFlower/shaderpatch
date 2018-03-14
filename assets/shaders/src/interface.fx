
#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"

float4 interface_transform_const : register(vs, c[CUSTOM_CONST_MIN]);
float4 ps_constant : register(ps, c[0]);

sampler bitmap_sampler;
sampler mask_sampler;

float4 project_interface_pos(float4 position)
{
   position = position_to_world_project(position);

   position.xy = position.xy * interface_transform_const.xy + interface_transform_const.zw;

   return position;
}

// Masked Bitmap Element

struct Vs_masked_input
{
   float4 position : POSITION;
   float4 texcoord_0 : TEXCOORD0;
   float4 texcoord_1 : TEXCOORD1;
};

struct Vs_masked_output
{
   float4 position : POSITION;
   float2 texcoord_0 : TEXCOORD0;
   float2 texcoord_1 : TEXCOORD1;
};

struct Ps_masked_input
{
   float2 texcoord_0 : TEXCOORD0;
   float2 texcoord_1 : TEXCOORD1;
};

Vs_masked_output masked_bitmap_vs(Vs_masked_input input)
{
   Vs_masked_output output;
    
   output.position = project_interface_pos(input.position);

   output.texcoord_0 = decompress_texcoords(input.texcoord_0);
   output.texcoord_1 = decompress_texcoords(input.texcoord_1);

   return output;
}

float4 masked_bitmap_ps(Ps_masked_input input) : COLOR
{
   float4 color = tex2D(bitmap_sampler, input.texcoord_0);

   return color * ps_constant * tex2D(mask_sampler, input.texcoord_1);
}

// Vector Element

struct Vs_vector_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_vector_output
{
   float4 position : POSITION;
   float4 color : COLOR;
};

Vs_vector_output vector_vs(Vs_vector_input input)
{
   Vs_vector_output output;

   output.position = project_interface_pos(input.position);
   output.color = input.color;

   return output;
}

float4 vector_ps(float4 color : COLOR) : COLOR
{
   return color * ps_constant;
}

// Bitmap Untextured

void bitmap_untextured_vs(inout float4 position : POSITION)
{
   position = project_interface_pos(position);
}

float4 bitmap_untextured_ps() : COLOR
{
   return ps_constant;
}

// Bitmap Textured

struct Vs_textured_input
{
   float4 position : POSITION;
   float4 texcoord : TEXCOORD;
};

struct Vs_textured_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

Vs_textured_output bitmap_textured_vs(Vs_textured_input input)
{
   Vs_textured_output output;

   output.position = project_interface_pos(input.position);

   output.texcoord = decompress_texcoords(input.texcoord);

   return output;
}

float4 bitmap_textured_ps(float2 texcoord : TEXCOORD) : COLOR
{
   return tex2D(bitmap_sampler, texcoord) * ps_constant;
}
