#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

float4 x_texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 0]);
float4 y_texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);

sampler diffuse_map_sampler;

struct Vs_opaque_input
{
   float4 position : POSITION;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
};

float4 opaque_vs(Vs_opaque_input input) : POSITION
{
   return transform::position_project(input.position, input.blend_indices, input.weights);
}

struct Vs_hardedged_input
{
   float4 position : POSITION;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
   float4 texcoords : TEXCOORD;
};

struct Vs_hardedged_output
{
   float4 position : POSITION;
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
};

Vs_hardedged_output hardedged_vs(Vs_hardedged_input input)
{
   Vs_hardedged_output output;

   output.position = transform::position_project(input.position, input.blend_indices, 
                                                 input.weights);
   output.texcoords = decompress_transform_texcoords(input.texcoords, x_texcoord_transform,
                                                     y_texcoord_transform);
   output.color = material_diffuse_color;

   return output;
}

// Pixel Shaders

float4 opaque_ps() : COLOR
{
   return float4(0.0, 0.0, 0.0, 1.0);
}

struct Ps_hardedged_input
{
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
};

float4 hardedged_ps(Ps_hardedged_input input) : COLOR
{
   return tex2D(diffuse_map_sampler, input.texcoords) * input.color;
}
