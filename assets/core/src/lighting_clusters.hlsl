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

struct Point_light {
   float3 positionWS;
   float inv_range_sq;
   float3 color;
   uint padding;
};

struct Spot_light {
   float3 positionWS;
   float inv_range_sq;
   float3 directionWS;
   float cone_outer_param;
   float3 color;
   float cone_inner_param;
};

StructuredBuffer<Cluster_index> light_clusters_index : register(t8);
StructuredBuffer<uint> light_clusters_lists : register(t9);
StructuredBuffer<Point_light> light_clusters_point_lights : register(t10);
StructuredBuffer<Spot_light> light_clusters_spot_lights : register(t11);

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

   positionCS.xy = (uint2)(positionSS.xy / float2(160, 180));
   positionCS.z = log2(position_zVS / near_cluster_z) / log2(far_cluster_z / near_cluster_z) * z_mul;
   positionCS.z = min(positionCS.z, z_clusters -1);

   return light_clusters_index[positionCS.z * clusters_slice + positionCS.y * x_clusters + positionCS.x];
}

}

#endif // LIGHTING_CLUSTERS_INCLUDED