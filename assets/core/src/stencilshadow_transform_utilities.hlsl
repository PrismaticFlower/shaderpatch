#ifndef STENCILSHADOW_TRANSFORM_UTILITIES_INCLUDED
#define STENCILSHADOW_TRANSFORM_UTILITIES_INCLUDED

#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"

float3 unskinned_positionWS(int4 position)
{
   const float3 positionOS = decompress_position((float3)position.xyz);

   return mul(float4(positionOS, 1.0), world_matrix);
}

float3 unskinned_normalWS(unorm float4 normal)
{
   const float3 normalOS = normal.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   return mul(normalOS, (float3x3)world_matrix);
}

float3 hard_skinned_positionOS(int4 position, int bone_index)
{
   const float3 position_decompressed = decompress_position((float3)position.xyz);
   return mul(float4(position_decompressed, 1.0), bone_matrices[bone_index]);
}

float3 hard_skinned_positionWS(int4 position, int bone_index)
{
   const float3 positionOS = hard_skinned_positionOS(position, bone_index);

   return mul(float4(positionOS, 1.0), world_matrix);
}

float3 hard_skinned_normalWS(unorm float4 normal, int bone_index)
{
   const float3 normal_decompressed = normal.xyz * 255.0 / 127.0 - 128.0 / 127.0;
   const float3 normalOS = mul(normal_decompressed, (float3x3)bone_matrices[bone_index]);

   return mul(normalOS, (float3x3)world_matrix);
}

float4x3 get_soft_skinned_matrix(float4 blend_weights, int4 blend_indices)
{
   float4x3 skin;

   skin = blend_weights.x * bone_matrices[blend_indices.x];

   skin += (blend_weights.y *  bone_matrices[blend_indices.y]);

   const float z_weight = 1 - blend_weights.x - blend_weights.y;

   skin += (z_weight *  bone_matrices[blend_indices.z]);

   return skin;
}

float3 soft_skinned_positionWS(int4 position, float4 blend_weights, int4 blend_indices)
{
   const float4x3 skin_matrix = get_soft_skinned_matrix(blend_weights, blend_indices);

   const float3 position_decompressed = decompress_position((float3)position.xyz);
   const float3 positionOS = mul(float4(position_decompressed, 1.0), skin_matrix);

   return mul(float4(positionOS, 1.0), world_matrix);
}

float3 soft_skinned_normalWS(unorm float4 normal, float4 blend_weights, int4 blend_indices)
{
   const float4x3 skin_matrix = get_soft_skinned_matrix(blend_weights, blend_indices);

   const float3 normal_decompressed = normal.xyz * 255.0 / 127.0 - 128.0 / 127.0;
   const float3 normalOS = mul(normal_decompressed, (float3x3)skin_matrix);

   return mul(normalOS, (float3x3)world_matrix);
}



#endif