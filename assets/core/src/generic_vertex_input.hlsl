#ifndef __STD_VERTEX_INPUT_INCLUDED__
#define __STD_VERTEX_INPUT_INCLUDED__

#include "constants_list.hlsl"

namespace detail {
namespace vi {
#ifdef __VERTEX_INPUT_IS_COMPRESSED__
typedef int4 Position_vec;
typedef unorm float4 Blend_indices_vec;
typedef unorm float4 Blend_weight_vec;
typedef unorm float4 Normal_vec;
typedef unorm float4 Color_vec;
typedef int2 Texcoords_vec;
#else
      typedef float3 Position_vec;
typedef unorm float4 Blend_indices_vec;
typedef float2 Blend_weight_vec;
typedef float3 Normal_vec;
typedef unorm float4 Color_vec;
typedef float2 Texcoords_vec;
#endif
}
}

struct Vertex_input
{
   float3 position()
   {
#  ifdef __VERTEX_INPUT_POSITION__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
         const float3 position = float3(_position.xyz);

         return position_decompress_max.xyz + (position_decompress_min.xyz * position);
      #else
         return _position;
      #endif
#  else
      return float3(0.0, 0.0, 0.0);
#  endif
   }

   float3 blend_weights()
   {
      if (vs_use_soft_skinning) 
      {
#        ifdef __VERTEX_INPUT_BLEND_WEIGHT__
               return float3(_blend_weights.x, _blend_weights.y, 1.0 - _blend_weights.x - _blend_weights.y);
#        else
            return float3(1.0, 0.0, 0.0);
#        endif
      }
      else
      {
         return float3(1.0, 0.0, 0.0);
      }
   }
   
   int3 bone_index()
   {
#  ifdef __VERTEX_INPUT_BLEND_INDICES__
      return int3(_blend_indices.xyz * 255);
#  else
      return int3(0, 0, 0);
#  endif
   }
   
   float3 normal()
   {
#  ifdef __VERTEX_INPUT_NORMAL__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
         return _normal.xyz * (255.0 / 127.0) - (128.0 / 127.0);
      #else
         return _normal;
      #endif
#  else
      return float3(0.0, 0.0, 0.0);
#  endif
   }

   float3 tangent()
   {
#  ifdef __VERTEX_INPUT_TANGENTS__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
         return _tangent.xyz * (255.0 / 127.0) - (128.0 / 127.0);
      #else
         return _tangent;
      #endif
#  else
      return float3(0.0, 0.0, 0.0);
#  endif
   }
   
   float3 bitangent()
   {
#  ifdef __VERTEX_INPUT_TANGENTS__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
         return _bitangent.xyz * (255.0 / 127.0) - (128.0 / 127.0);
      #else
         return _bitangent;
      #endif
#  else
      return float3(0.0, 0.0, 0.0);
#  endif
   }

   float3 patch_tangent()
   {
#  ifdef __VERTEX_INPUT_TANGENTS__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
      return _tangent.xyz * (255.0 / 127.0) - (128.0 / 127.0);
#else
      return _tangent;
#endif
#  else
      return float3(0.0, 0.0, 0.0);
#  endif
   }

   float patch_bitangent_sign()
   {
#  ifdef __VERTEX_INPUT_TANGENTS__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
      return sign(_tangent.w - 0.5);
#else
      return _bitangent.z;
#endif
#  else
      return 0.0f;
#  endif
   }

   float4 color()
   {
#  ifdef __VERTEX_INPUT_COLOR__
      return _color;
#  else
      return float4(0.0, 0.0, 0.0, 1.0);
#  endif
   }
   
   float2 texcoords()
   {
#  ifdef __VERTEX_INPUT_TEXCOORDS__
#     ifdef __VERTEX_INPUT_IS_COMPRESSED__
         return (float2)_texcoords / 2048.0;
      #else
         return _texcoords;
      #endif
#  else
      return float2(0.0, 0.0);
#  endif
   }

#ifdef __VERTEX_INPUT_POSITION__
   detail::vi::Position_vec _position : POSITION;
#endif

#ifdef __VERTEX_INPUT_BLEND_WEIGHT__
   detail::vi::Blend_weight_vec _blend_weights : BLENDWEIGHT;
#endif
   
#ifdef __VERTEX_INPUT_BLEND_INDICES__
   detail::vi::Blend_indices_vec _blend_indices : BLENDINDICES;
#endif

#ifdef __VERTEX_INPUT_NORMAL__
   detail::vi::Normal_vec _normal : NORMAL;
#endif

#ifdef __VERTEX_INPUT_TANGENTS__
   detail::vi::Normal_vec _tangent : TANGENT;
   detail::vi::Normal_vec _bitangent   : BINORMAL;
#endif

#ifdef __VERTEX_INPUT_COLOR__
   detail::vi::Color_vec _color : COLOR;
#endif

#ifdef __VERTEX_INPUT_TEXCOORDS__
   detail::vi::Texcoords_vec _texcoords : TEXCOORD;
#endif
};

#endif