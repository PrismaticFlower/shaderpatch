#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

float4 x_texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 0]);
float4 y_texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);

sampler2D diffuse_map_sampler;

struct Vs_input
{
   float4 position : POSITION;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
   float4 texcoords : TEXCOORD;
};

struct Vs_output
{
   float4 position : POSITION;
   float camera_distance : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
};

Vs_output prepass_vs(Vs_input input)
{
   Vs_output output;

   float4 world_position = transform::position(input.position, input.blend_indices, 
                                               input.weights);

   output.position = position_project(world_position);
   output.texcoords = decompress_transform_texcoords(input.texcoords, x_texcoord_transform,
                                                     y_texcoord_transform);
   output.camera_distance = distance(world_view_position, world_position.xyz);

   return output;
}

struct Ps_input
{
   float camera_distance : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
};

float4 opaque_ps(Ps_input input) : COLOR
{
   return input.camera_distance;
}

float4 hardedged_ps(Ps_input input) : COLOR
{
   float alpha = (tex2D(diffuse_map_sampler, input.texcoords) * material_diffuse_color).a;

   if (alpha < 0.5) discard;

   return input.camera_distance;
}
