#ifndef TERRAIN_COMMON_INCLUDED
#define TERRAIN_COMMON_INCLUDED

#include "vertex_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

const static bool terrain_common_use_shadow_map = TERRAIN_COMMON_USE_SHADOW_MAP;
const static bool terrain_common_use_proj_light = TERRAIN_COMMON_USE_PROJ_LIGHT;
const static bool terrain_common_use_parallax_offset_mapping = TERRAIN_COMMON_USE_PARALLAX_OFFSET_MAPPING;
const static bool terrain_common_use_parallax_occlusion_mapping = TERRAIN_COMMON_USE_PARALLAX_OCCLUSION_MAPPING;
const static bool terrain_common_low_detail = TERRAIN_COMMON_LOW_DETAIL;


struct Packed_terrain_vertex {
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
   unorm float4 tangent : TANGENT;
};

struct Terrain_vertex {
   float3 positionWS;
   float3 color;
   float2 terrain_coords;
   uint3 texture_indices;
   float2 texture_blend;
};

Terrain_vertex unpack_vertex(Packed_terrain_vertex packed, const bool srgb_colors)
{
   Terrain_vertex unpacked;

   unpacked.positionWS = decompress_position((float3)packed.position.xyz);

   unpacked.texture_indices[0] = (packed.position.w) & 0xf;
   unpacked.texture_indices[1] = (packed.position.w >> 4) & 0xf;
   unpacked.texture_indices[2] = (packed.position.w >> 8) & 0xf;

   unpacked.texture_blend[0] = packed.normal.x;
   unpacked.texture_blend[1] = packed.normal.y;

   unpacked.color = float3(packed.tangent.rgb);

   if (srgb_colors) unpacked.color = srgb_to_linear(unpacked.color);

   unpacked.terrain_coords = max((float2)packed.position.xz / 32767.0f, -1.0) * float2(1.0, -1.0) * 0.5 + 0.5;

   return unpacked;
}

float3x3 terrain_tangent_to_world(const float3 normalWS)
{
   const float3 tangentWS = normalize(-normalWS.x * normalWS + float3(1, 0, 0));
   const float3 bitangentWS = normalize(-normalWS.z * normalWS + float3(0, 0, 1));

   return float3x3(tangentWS, bitangentWS, normalWS);
}

float3x3 terrain_sample_normal_map(Texture2D<float3> terrain_normal_map, const float2 terrain_coords)
{
   float3 normalWS = terrain_normal_map.Sample(aniso_wrap_sampler, terrain_coords) * (1024.0 / 511.0) - (512.0 / 511.0);
   normalWS = normalize(normalWS);

   return terrain_tangent_to_world(normalWS);
}

class Terrain_parallax_texture : Parallax_input_texture {
   float CalculateLevelOfDetail(SamplerState samp, float2 texcoords)
   {
      return height_maps.CalculateLevelOfDetail(samp, texcoords);
   }

   float SampleLevel(SamplerState samp, float2 texcoords, float mip)
   {
      return height_maps.SampleLevel(samp, float3(texcoords, index), mip);
   }

   float Sample(SamplerState samp, float2 texcoords)
   {
      return height_maps.Sample(samp, float3(texcoords, index));
   }

   float index;
   Texture2DArray<float1> height_maps;
};

void terrain_sample_heightmaps(Texture2DArray<float1> height_maps, const float3 height_scales, 
                               const float3 unorm_viewTS, const float3 vertex_texture_blend,
                               inout float3 texcoords[3], out float3 heights)
{
   Terrain_parallax_texture parallax_height_map;
   parallax_height_map.height_maps = height_maps;

   heights = 0.0;

   if (terrain_common_low_detail) return;

   [unroll] for (int i = 0; i < 3; ++i) {
      [branch] if (vertex_texture_blend[i] > 0.0) {
         parallax_height_map.index = texcoords[i].z;

         if (terrain_common_use_parallax_offset_mapping) {
            texcoords[i].xy =
               parallax_offset_map(parallax_height_map, height_scales[i],
                                   texcoords[i].xy, normalize(unorm_viewTS));
         }
         else if (terrain_common_use_parallax_occlusion_mapping) {
            texcoords[i].xy =
               parallax_occlusion_map(parallax_height_map, height_scales[i],
                                      texcoords[i].xy, unorm_viewTS);
         }

         heights[i] = height_maps.Sample(aniso_wrap_sampler, texcoords[i]);
      }
   }
}

float3 terrain_blend_weights(const float3 heights, const float3 vertex_texture_blend)
{
   const float offset = 0.2;

   const float3 h = heights + vertex_texture_blend;
   const float offset_max_h = max(max(h[0], h[1]), h[2]) - offset;

   const float3 b = max(h - offset_max_h, 0.0);

   return (1.0 * b) / max(b[0] + b[1] + b[2], 1e-5);
}

#endif