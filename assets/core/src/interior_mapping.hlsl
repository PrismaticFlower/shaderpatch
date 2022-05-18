#ifndef INTERIOR_MAPPING_INCLUDED
#define INTERIOR_MAPPING_INCLUDED

#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

// Adaptation of Andrew Gotow's technique from here: https://andrewgotow.com/2018/09/09/interior-mapping-part-2/
float3 interior_map(float3 viewTS, float2 texcoords, float3 interior_spacing,
                    float3x3 tangent_to_world, TextureCube<float3> interior_map)
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

   return interior_map.Sample(aniso_wrap_sampler, hit_directionOS);
}

#endif