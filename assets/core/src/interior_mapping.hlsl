#ifndef INTERIOR_MAPPING_INCLUDED
#define INTERIOR_MAPPING_INCLUDED

#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

uint pcg_hash(uint input)
{
   uint state = input * 747796405u + 2891336453u;
   uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
   return (word >> 22u) ^ word;
}

uint interior_texture_index(float2 texcoords, float3 interior_spacing, float2 array_size_info)
{
   uint2 room_index = floor(texcoords / interior_spacing.xy);
   uint hash = pcg_hash(room_index.x | (room_index.y << 16u));

   float inv_array_size = array_size_info.x;
   float array_size = array_size_info.y;

   float random_number = (float)(hash & 0xffffffu);
   uint texture_index =
      random_number - (floor(random_number * inv_array_size) * array_size);

   return texture_index;
}

// Adaptation of Andrew Gotow's technique from here: https://andrewgotow.com/2018/09/09/interior-mapping-part-2/
float3 interior_map(float3 viewTS, float2 texcoords, float3 interior_spacing,
                    float3x3 tangent_to_world,
                    TextureCubeArray<float3> interior_map, float2 array_size_info)
{
   float3 ray_originTS = frac(float3(texcoords, 0.0) / interior_spacing);
   float3 ray_directionTS = viewTS / interior_spacing;

   float3 box_minTS = floor(float3(texcoords, -1.0));
   float3 box_maxTS = box_minTS + 1.0;
   float3 box_midTS = box_minTS + 0.5;

   float3 planesTS = lerp(box_minTS, box_maxTS, step(0.0, ray_directionTS));
   float3 hit_planes = (planesTS - ray_originTS) / ray_directionTS;

   float hit_distance = min(min(hit_planes.x, hit_planes.y), hit_planes.z);

   float3 hit_directionTS = (ray_originTS + ray_directionTS * hit_distance) - box_midTS;

   // Sample in object space for nicer cubemap orientation.
   float3 hit_directionWS = mul(hit_directionTS, tangent_to_world);
   float3 hit_directionOS = mul((float3x3)world_matrix, hit_directionWS);

   uint texture_index =
      interior_texture_index(texcoords, interior_spacing, array_size_info);

   return interior_map.Sample(aniso_wrap_sampler,
                              float4(hit_directionOS, texture_index));
}

#endif