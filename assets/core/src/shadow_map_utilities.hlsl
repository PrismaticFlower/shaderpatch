#ifndef SHADOW_MAP_UTILS_INCLUDED
#define SHADOW_MAP_UTILS_INCLUDED

#include "pixel_sampler_states.hlsl"
#include "constants_list.hlsl"

Texture2DArray<float> directional_light0_shadow_map : register(t7);

const static uint shadow_cascade_count = 4;

float shadow_cascade_signed_distance(float3 positionLS)
{
   float3 centered_positionLS = (positionLS - 0.5);
   float3 distance = abs(centered_positionLS) - 0.5;

   return max(max(distance.x, distance.y), distance.z);
}

void select_shadow_map_cascade(float3 positionWS,
                               float4x4 shadow_matrices[4],
                               out uint out_cascade_index,
                               out float out_cascade_signed_distance,
                               out float3 out_positionLS)
{
   uint cascade_index = shadow_cascade_count - 1;

   for (int i = shadow_cascade_count - 2; i >= 0; --i)
   {
      float3 positionLS = mul(float4(positionWS, 1.0), shadow_matrices[i]).xyz;

      float cascade_signed_distance = shadow_cascade_signed_distance(positionLS);

      if (shadow_cascade_signed_distance(positionLS) <= 0.0) {
         cascade_index = i;
         out_cascade_signed_distance = cascade_signed_distance;
         out_positionLS = positionLS;
      }
   }

   if (cascade_index == shadow_cascade_count - 1) {
         out_cascade_signed_distance = -0.5;
         out_positionLS = mul(float4(positionWS, 1.0), shadow_matrices[cascade_index]).xyz;
   }

   out_cascade_index = cascade_index;
}

float sample_shadow_map_cascade(Texture2DArray<float> shadow_map, float2 positionLS, float light_depth,
                                uint cascade_index, float inv_shadow_map_resolution)
{
   // Sampling pattern from MiniEngine https://github.com/microsoft/DirectX-Graphics-Samples/blob/0aa79bad78992da0b6a8279ddb9002c1753cb849/MiniEngine/Model/Shaders/Lighting.hlsli#L70

   const float dilation = 2.0;

   float d1 = dilation * inv_shadow_map_resolution * 0.125;
   float d2 = dilation * inv_shadow_map_resolution * 0.875;
   float d3 = dilation * inv_shadow_map_resolution * 0.625;
   float d4 = dilation * inv_shadow_map_resolution * 0.375;

   float shadow =
      (2.0 * shadow_map.SampleCmpLevelZero(shadow_sampler, float3(positionLS.xy, cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d2, d1), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d1, -d2), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d2, -d1), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d1, d2), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d4, d3), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d3, -d4), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d4, -d3), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler, float3(positionLS.xy + float2(d3, d4), cascade_index),
                                     light_depth).r) /
      10.0;

   return shadow;
}


float sample_cascaded_shadow_map(Texture2DArray<float> shadow_map, float3 positionWS, float shadow_texel_size, 
                                 float shadow_bias, float4x4 shadow_matrices[4])
{
   uint cascade_index;
   float cascade_signed_distance;
   float3 positionLS;

   select_shadow_map_cascade(positionWS, shadow_matrices, cascade_index, cascade_signed_distance, positionLS);

   // TODO: Offset UV by a half texel?

   float shadow = sample_shadow_map_cascade(shadow_map, positionLS.xy, positionLS.z + shadow_bias, 
                                            cascade_index, shadow_texel_size);
   
   [branch]
   if (cascade_index != 3 && cascade_signed_distance >= cascade_fade_distance) {
      const uint next_cascade_index = cascade_index + 1;
      const float fade = cascade_signed_distance * inv_cascade_fade_distance;
      
      float3 next_positionLS = mul(float4(positionWS, 1.0), shadow_matrices[next_cascade_index]).xyz;

      float next_shadow = sample_shadow_map_cascade(shadow_map, next_positionLS.xy, next_positionLS.z + shadow_bias, 
                                                    next_cascade_index, shadow_texel_size);

      shadow = lerp(next_shadow, shadow, smoothstep(0.0, 1.0, saturate(fade)));
   }

   return shadow;
}


#endif