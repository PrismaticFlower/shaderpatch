#ifndef TRANSFORM_UTILS_INCLUDED
#define TRANSFORM_UTILS_INCLUDED

#pragma warning(disable : 3206)

#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"

namespace transform
{

namespace unskinned
{

//! \brief Decompress an unskinned vertex position.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices Ignored.
//! \param weights Ignored.
//!
//! \return The object space position of the vertex.
float4 position(float4 position, uint4 indices, float4 weights)
{
   return decompress_position(position);
}

//! \brief Decompress an unskinned vertex normal.
//!
//! \param normal The (potentially compressed) vertex normal.
//! \param indices Ignored.
//! \param weights Ignored.
//!
//! \return The object space normal of the vertex.
float3 normals(float3 normal, uint4 indices, float4 weights)
{
   return decompress_normals(normal);
}

//! \brief Decompress unskinned vertex binormals.
//!
//! \param binormal The (potentially compressed) vertex binormal.
//! \param tangent The (potentially compressed) vertex tangent.
//! \param indices Ignored.
//! \param weights Ignored.
//!
//! \return The object space binormals of the vertex.
Binormals binormals(float3 binormal, float3 tangent, uint4 indices, float4 weights)
{
   return decompress_binormals(binormal, tangent);
}

}

namespace soft_skinned
{

//! \brief Gets the transformation matrix for a bone
//!
//! \param index The index of the bone.
//!
//! \return The transformation matrix of the bone.
float3x4 get_bone_matrix(int index)
{
   float3x4 mat;

   mat[0] = bone_matrices[0 + index];
   mat[1] = bone_matrices[1 + index];
   mat[2] = bone_matrices[2 + index];

   return mat;
}

//! \brief Calculate combined transformation matrix for 
//! a soft skinned vertex.
//!
//! \param index The index of the bone.
//! \param weights The vertex weights.
//! \param vertex_indices The (potentially compressed) vertex blend indices.
//!
//! \return The soft skin transformation matrix of the vertex.
float3x4 calculate_skin(float4 weights, uint4 vertex_indices)
{
   int3 indices = vertex_indices.xyz * 765.001;
   
   float3x4 skin;

   skin = weights.x * get_bone_matrix(indices.x);

   skin += (weights.y *  get_bone_matrix(indices.y));

   float z_weight = 1 - weights.x - weights.y;

   skin += (z_weight *  get_bone_matrix(indices.z));

   return skin;
}

//! \brief Decompress and transform a skinned vertex position to object
//! space.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space position of the vertex.
float4 position(float4 position, uint4 indices, float4 weights)
{
   position = decompress_position(position);

   float3x4 skin = calculate_skin(weights, indices);

   return float4(mul(skin, position), 1.0);
}

//! \brief Decompress and transform a skinned vertex normal to object
//! space.
//!
//! \param normal The (potentially compressed) vertex normal.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space normal of the vertex.
float3 normals(float3 normal, uint4 indices, float4 weights)
{
   normal = decompress_normals(normal);
   
   float3x4 skin = calculate_skin(weights, indices);

   float3 obj_normal = mul(skin, normal);

   return normalize(obj_normal);
}

//! \brief Decompress and transform skinned vertex binormals to object
//! space.
//!
//! \param binormal The (potentially compressed) vertex binormal.
//! \param tangent The (potentially compressed) vertex tangent.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space binormals of the vertex.
Binormals binormals(float3 binormal, float3 tangent, uint4 indices, float4 weights)
{
   Binormals binormals = decompress_binormals(binormal, tangent);

   float3x4 skin = calculate_skin(weights, indices);

   Binormals obj_binormals;

   obj_binormals.s = mul(skin, binormals.s);
   obj_binormals.t = mul(skin, binormals.t);

   obj_binormals.s = normalize(obj_binormals.s);
   obj_binormals.t = normalize(obj_binormals.t);
   
   return obj_binormals;
}

}

namespace hard_skinned
{

//! \brief Decompress and transform a hard skinned vertex position 
//! to object space. Provided for the stencilshadow shader.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices The (potentially compressed) vertex blend indices.
//!
//! \return The object space position of the vertex.
float4 position(float4 position, uint4 indices)
{
   int index = indices.x * 765.001;

   position = decompress_position(position);

   float3x4 skin = soft_skinned::get_bone_matrix(index);

   return float4(mul(skin, position), 1.0);
}

//! \brief Decompress and transform a hard skinned vertex normal 
//! to object space. Provided for the stencilshadow shader.
//!
//! \param normal The (potentially compressed) vertex normal.
//! \param indices The (potentially compressed) vertex blend indices.
//!
//! \return The object space normal of the vertex.
float3 normals(float3 normals, uint4 indices)
{
   int index = indices.x * 765.001;

   normals = decompress_normals(normals);

   float3x4 skin = soft_skinned::get_bone_matrix(index);

   float3 obj_normal = (float3) mul(skin, normals);

   return normalize(obj_normal);
}

//! \brief Decompress and transform hard skinned vertex binormals
//! to object space.
//!
//! \param binormal The (potentially compressed) vertex binormal
//! \param tangent The (potentially compressed) vertex tangent.
//! \param indices The (potentially compressed) vertex blend indices.
//!
//! \return The object space binormals of the vertex.
Binormals binormals(float3 binormal, float3 tangent, uint4 indices)
{
   int index = indices.x * 765.001;

   Binormals binormals = decompress_binormals(binormal, tangent);

   float3x4 skin = soft_skinned::get_bone_matrix(index);

   Binormals obj_binormals;

   obj_binormals.s = mul(skin, binormals.s);
   obj_binormals.t = mul(skin, binormals.t);

   obj_binormals.s = normalize(obj_binormals.s);
   obj_binormals.t = normalize(obj_binormals.t);

   return obj_binormals;
}

}

#if defined(TRANSFORM_SOFT_SKINNED)
#define skin_type_position soft_skinned::position
#define skin_type_normals soft_skinned::normals
#define skin_type_binormals soft_skinned::binormals
#else
#define skin_type_position unskinned::position
#define skin_type_normals unskinned::normals
#define skin_type_binormals unskinned::binormals
#endif

//! \brief Decompress and transform a skinned or unskinned vertex
//! position; which operation is taken depends on if TRANSFORM_SOFT_SKINNED 
//! is defined or not.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The world space position of the vertex.
float4 position(float4 position, uint4 indices, float4 weights)
{
   float4 obj_position = skin_type_position(position, indices, weights);

   return position_to_world(obj_position);
}

//! \brief Decompress and transform a skinned or unskinned vertex
//! position; which operation is taken depends on if TRANSFORM_SOFT_SKINNED 
//! is defined or not.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space position of the vertex.
float4 position_obj(float4 position, uint4 indices, float4 weights)
{
   return skin_type_position(position, indices, weights);
}

//! \brief Decompress, transform and project a skinned or unskinned vertex
//! position; which operation is taken depends on if TRANSFORM_SOFT_SKINNED 
//! is defined or not.
//!
//! \param position The (potentially compressed) vertex position.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The projection space position of the vertex.
float4 position_project(float4 world_position, uint4 indices, float4 weights)
{
   return ::position_project(position(world_position, indices, weights));
}

//! \brief Decompress and transform a skinned or unskinned vertex
//! normal to object space; which operation is taken depends on
//! if TRANSFORM_SOFT_SKINNED is defined or not.
//!
//! \param normal The (potentially compressed) vertex normal.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space normal of the vertex.
float3 normals(float3 normal, uint4 indices, float4 weights)
{
   return skin_type_normals(normal, indices, weights);
}

//! \brief Decompress and transform a skinned or unskinned vertex's
//! binormals to object space; which operation is taken depends on
//! if TRANSFORM_SOFT_SKINNED is defined or not.
//!
//! \param binormal The (potentially compressed) vertex binormal.
//! \param tangent The (potentially compressed) vertex tangent.
//! \param indices The (potentially compressed) vertex blend indices.
//! \param weights The vertex weights.
//!
//! \return The object space binormals of the vertex.
Binormals binormals(float3 binormal, float3 tangent, uint4 indices, float4 weights)
{
   return skin_type_binormals(binormal, tangent, indices, weights);
}

#undef skin_type_position
#undef skin_type_normals
#undef skin_type_binormals

//! \brief Decompress and transform an unskinned vertex.
//!
//! \param position The (potentially compressed) vertex position.
//!
//! \return The world space position of the vertex.
float4 position(float4 position)
{
   return position_to_world(decompress_position(position));
}

//! \brief Decompress, transform and project an unskinned vertex.
//!
//! \param position The (potentially compressed) vertex position.
//!
//! \return The projection space position of the vertex.
float4 position_project(float4 position)
{
   return ::position_project(transform::position(position));
}

}

#pragma warning(default : 3206)

#endif