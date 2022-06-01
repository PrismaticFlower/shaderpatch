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

uint interior_room_randomness(float2 texcoords, float3 interior_spacing, uint hash_seed)
{
   const int max_rooms = 65536;

   int2 room_index = floor(texcoords / interior_spacing.xy);

   if (room_index.x < 0) room_index.x += max_rooms;
   if (room_index.y < 0) room_index.y += max_rooms;

   return pcg_hash((room_index.x | (room_index.y << 16u)) + hash_seed);
}

uint interior_texture_index(uint hash, float2 array_size_info)
{
   float inv_array_size = array_size_info.x;
   float array_size = array_size_info.y;

   float random_number = (float)(hash & 0xffffffu);
   uint texture_index =
      random_number - (floor(random_number * inv_array_size) * array_size);

   return texture_index;
}

// Adaptation of Andrew Gotow's technique from here: https://andrewgotow.com/2018/09/09/interior-mapping-part-2/
float3 interior_map(float3 viewTS, float2 unorm_texcoords,
                    float3 interior_spacing, uint hash_seed, bool randomize_walls,
                    TextureCubeArray<float3> interior_map, float2 array_size_info)
{
   uint randomness =
      interior_room_randomness(unorm_texcoords, interior_spacing, hash_seed);

   float2 texcoords = frac(unorm_texcoords);

   if (texcoords.x < 0) texcoords.x = 1.0 - texcoords.x;
   if (texcoords.y < 0) texcoords.y = 1.0 - texcoords.y;

   float3 ray_originTS = frac(float3(texcoords, 0.0) / interior_spacing);
   float3 ray_directionTS = viewTS / interior_spacing;

   float3 box_minTS = floor(float3(texcoords, -1.0));
   float3 box_maxTS = box_minTS + 1.0;
   float3 box_midTS = box_minTS + 0.5;

   float3 planesTS = lerp(box_minTS, box_maxTS, step(0.0, ray_directionTS));
   float3 hit_planes = (planesTS - ray_originTS) / ray_directionTS;

   float hit_distance = min(min(hit_planes.x, hit_planes.y), hit_planes.z);

   float3 hit_directionTS = (ray_originTS + ray_directionTS * hit_distance) - box_midTS;

   hit_directionTS.y = -hit_directionTS.y;

   if (randomize_walls) {
      if (randomness & 0x10000000) hit_directionTS.xz = hit_directionTS.zx;
      if (randomness & 0x20000000) hit_directionTS.x = -hit_directionTS.x;
      if (randomness & 0x40000000) hit_directionTS.z = -hit_directionTS.z;
   }

   uint texture_index = interior_texture_index(randomness, array_size_info);

   return interior_map.Sample(aniso_wrap_sampler,
                              float4(hit_directionTS, texture_index));
}

#endif