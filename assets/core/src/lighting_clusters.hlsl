#ifndef LIGHTING_CLUSTERS_INCLUDED
#define LIGHTING_CLUSTERS_INCLUDED

#include "constants_list.hlsl"

namespace light {

struct Cluster_index {
   uint point_lights_start;
   uint point_lights_end;
   uint spot_lights_start;
   uint spot_lights_end;
};

struct Cluster_point_light {
   float3 positionWS;
   float  inv_range_sq;
   float3 color;
   uint   options;
};

struct Cluster_spot_light {
   float3 positionWS;
   float  inv_range_sq;
   float3 directionWS;
   float  cone_outer_param;
   float3 color;
   float  cone_inner_param;
   uint   options;
   uint3  padding;
};

StructuredBuffer<Cluster_index> light_clusters_index : register(t8);
StructuredBuffer<uint> light_clusters_lists : register(t9);
StructuredBuffer<Cluster_point_light> light_clusters_point_lights : register(t10);
StructuredBuffer<Cluster_spot_light> light_clusters_spot_lights : register(t11);

StructuredBuffer<float4x4> light_clusters_texture_transforms : register(t12);

TextureCube<float3> light_clusters_point_texture_0 : register(t13);
TextureCube<float3> light_clusters_point_texture_1 : register(t14);
TextureCube<float3> light_clusters_point_texture_2 : register(t15);

Texture2D<float3> light_clusters_spot_texture_0 : register(t16);
Texture2D<float3> light_clusters_spot_texture_1 : register(t17);
Texture2D<float3> light_clusters_spot_texture_2 : register(t18);

Cluster_index load_cluster(float3 positionWS, float4 positionSS)
{
   const float near_cluster_z = 5.0f;
   const float far_cluster_z = 1000.0f;
   const float z_mul = 32.0;
   const float position_zVS = dot(float4(positionWS, 1.0), view_matrix_z);

   const uint x_clusters = 16;
   const uint y_clusters = 8;
   const uint z_clusters = 32;
   const uint clusters_slice = x_clusters * y_clusters;  

   uint3 positionCS;

   positionCS.xy = (uint2)(positionSS.xy * inv_cluster_xy_length);
   positionCS.z = log2(position_zVS / near_cluster_z) / log2(far_cluster_z / near_cluster_z) * z_mul;
   positionCS.z = min(positionCS.z, z_clusters -1);

   return light_clusters_index[positionCS.z * clusters_slice + positionCS.y * x_clusters + positionCS.x];
}

const static uint light_options_static_bit = 0u;
const static uint light_options_has_texture_bit = 1u;
const static uint light_options_has_shadow_bit = 2u;

const static uint light_options_texture_transform_offset = 3u;
const static uint light_options_texture_transform_bits = 13u;
const static uint light_options_texture_transform_mask = (1u << light_options_texture_transform_bits) - 1u;

const static uint light_options_texture_index_offset = 16u;
const static uint light_options_texture_index_bits = 2u;
const static uint light_options_texture_index_mask = (1u << light_options_texture_index_bits) - 1u;

const static uint light_options_shadow_data_offset = 18u;
const static uint light_options_shadow_data_bits = 13u;
const static uint light_options_shadow_data_mask = (1u << light_options_texture_transform_bits) - 1u;

bool options_static(uint options)
{
   return (options & (1u << light_options_static_bit)) != 0;
}

bool options_has_texture(uint options)
{
   return (options & (1u << light_options_has_texture_bit)) != 0;
}

bool options_has_shadow(uint options)
{
   return (options & (1u << light_options_has_shadow_bit)) != 0;
}

uint options_texture_transform(uint options)
{
   return (options >> light_options_texture_transform_offset) & light_options_texture_transform_mask;
}

uint options_texture_index(uint options)
{
   return (options >> light_options_texture_index_offset) & light_options_texture_index_mask;
}

uint options_shadow_data(uint options)
{
   return (options >> light_options_shadow_data_offset) & light_options_shadow_data_mask;
}


}

#endif // LIGHTING_CLUSTERS_INCLUDED