
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"

interface Transformer
{
   float3 positionOS();

   float3 positionWS();

   float4 positionPS();

   float3 normalOS();

   float3 normalWS();

   float3 tangentOS();

   float3 tangentWS();

   float3 bitangentOS();

   float3 bitangentWS();

   float3 patch_tangentOS();

   float3 patch_tangentWS();

   float2 texcoords(float4 x_transform, float4 y_transform);
};

Transformer create_transformer(Vertex_input input);

class Transformer_unskinned : Transformer
{
   float3 positionOS()
   {
      return input.position();
   }

   float3 positionWS()
   {
      return mul(float4(positionOS(), 1.0), world_matrix);
   }

   float4 positionPS()
   {
      return mul(float4(positionWS(), 1.0), projection_matrix);
   }

   float3 normalOS()
   {
      return input.normal();
   }

   float3 normalWS()
   {
      return normalize(mul(normalOS(), (float3x3)world_matrix));
   }

   float3 tangentOS()
   {
      return input.tangent();
   }

   float3 tangentWS()
   {
      return normalize(mul(tangentOS(), (float3x3)world_matrix));
   }

   float3 bitangentOS()
   {
      return input.bitangent();
   }

   float3 bitangentWS()
   {
      return normalize(mul(bitangentOS(), (float3x3)world_matrix));
   }

   float3 patch_tangentOS()
   {
      return input.patch_tangent();
   }

   float3 patch_tangentWS()
   {
      return normalize(mul(patch_tangentOS(), (float3x3)world_matrix));
   }

   float2 texcoords(float4 x_transform, float4 y_transform)
   {
      return transform_texcoords(input.texcoords(), x_transform, y_transform);
   }

   Vertex_input input;
};

class Transformer_hard_skinned : Transformer
{
   float3 positionOS()
   {
      return mul(float4(input.position(), 1.0), skin_matrix);
   }

   float3 positionWS()
   {
      return mul(float4(positionOS(), 1.0), world_matrix);
   }

   float4 positionPS()
   {
      return mul(float4(positionWS(), 1.0), projection_matrix);
   }

   float3 normalOS()
   {
      return mul(input.normal(), (float3x3)skin_matrix);
   }

   float3 normalWS()
   {
      return normalize(mul(normalOS(), (float3x3)world_matrix));
   }

   float3 tangentOS()
   {
      return mul(input.tangent(), (float3x3)skin_matrix);
   }

   float3 tangentWS()
   {
      return normalize(mul(tangentOS(), (float3x3)world_matrix));
   }

   float3 bitangentOS()
   {
      return mul(input.bitangent(), (float3x3)skin_matrix);
   }

   float3 bitangentWS()
   {
      return normalize(mul(bitangentOS(), (float3x3)world_matrix));
   }

   float3 patch_tangentOS()
   {
      return mul(input.patch_tangent(), (float3x3)skin_matrix);
   }

   float3 patch_tangentWS()
   {
      return normalize(mul(patch_tangentOS(), (float3x3)world_matrix));
   }

   float2 texcoords(float4 x_transform, float4 y_transform)
   {
      return transform_texcoords(input.texcoords(), x_transform, y_transform);
   }

   Vertex_input input;
   float4x3 skin_matrix;
};

class Transformer_null : Transformer
{
   float3 positionOS()
   {
      return 0.0;
   }

   float3 positionWS()
   {
      return 0.0;
   }

   float4 positionPS()
   {
      return 0.0;
   }

   float3 normalOS()
   {
      return 0.0;
   }

   float3 normalWS()
   {
      return 0.0;
   }

   float3 tangentOS()
   {
      return 0.0;
   }

   float3 tangentWS()
   {
      return 0.0;
   }

   float3 bitangentOS()
   {
      return 0.0;
   }

   float3 bitangentWS()
   {
      return 0.0;
   }

   float3 patch_tangentOS()
   {
      return 0.0;
   }

   float3 patch_tangentWS()
   {
      return 0.0;
   }

   float2 texcoords(float4 x_transform, float4 y_transform)
   {
      return 0.0;
   }

   Vertex_input input;
};

Transformer create_transformer(Vertex_input input)
{
#ifdef __VERTEX_TRANSFORM_UNSKINNED__
   Transformer_unskinned transformer;
#elif defined(__VERTEX_TRANSFORM_HARD_SKINNED__)
   Transformer_hard_skinned transformer;
   
   int3 bone_index = input.bone_index();
   float3 weights = input.blend_weights();
   
   float4x3 skin_matrix;
   
   skin_matrix = bone_matrices[bone_index.x] * weights.x;
   skin_matrix += bone_matrices[bone_index.y] * weights.y;
   skin_matrix += bone_matrices[bone_index.z] * weights.z;
   
   transformer.skin_matrix = skin_matrix;
   
#elif !defined(__VERTEX_SHADER__)
   Transformer_null transformer;
#else
#error vertex_transformer.hlsl included in a vertex shader without a __VERTEX_TRANSFORM_*__ definition
#endif

   transformer.input = input;

   return transformer;
}
