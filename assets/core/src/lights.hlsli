#ifndef LIGHTS_INCLUDED
#define LIGHTS_INCLUDED

#include "constants_list.hlsl"
#include "lighting_clusters.hlsl"

#ifndef SP_USE_PROJECTED_TEXTURE
#define SP_USE_PROJECTED_TEXTURE 0
#endif

#ifndef SP_USE_STENCIL_SHADOW_MAP
#define SP_USE_STENCIL_SHADOW_MAP 0
#endif

#ifndef SP_USE_ADVANCED_LIGHTING
#define SP_USE_ADVANCED_LIGHTING 0
#endif

#define SP_HAS_STENCIL_SHADOW_FACTOR SP_USE_STENCIL_SHADOW_MAP && !SP_USE_ADVANCED_LIGHTING

#define SP_HAS_SUN_SHADOW_MAP SP_USE_ADVANCED_LIGHTING


struct Directional_light {
   float3 directionWS;
   float3 color;
   
#if SP_USE_PROJECTED_TEXTURE
   bool use_projected_texture() { return _use_projected_texture; }

   bool _use_projected_texture;
#else
   bool use_projected_texture() { return false; }
#endif

#if SP_HAS_STENCIL_SHADOW_FACTOR
   float stencil_shadow_factor() { return _stencil_shadow_factor; }

   float _stencil_shadow_factor;
#else
   float stencil_shadow_factor() { return 0.0; }
#endif

#if SP_HAS_SUN_SHADOW_MAP
   bool use_sun_shadow_map() { return _use_sun_shadow_map; }

   bool _use_sun_shadow_map;
#else
   bool use_sun_shadow_map() { return false; }
#endif
};

struct Point_light {
   float3 positionWS;
   float inv_range_sq;
   float3 color;

#if SP_USE_PROJECTED_TEXTURE
   bool use_projected_texture() { return _use_projected_texture; }

   bool _use_projected_texture;
#else
   bool use_projected_texture() { return false; }
#endif

#if SP_HAS_STENCIL_SHADOW_FACTOR
   float stencil_shadow_factor() { return _stencil_shadow_factor; }

   float _stencil_shadow_factor;
#else
   float stencil_shadow_factor() { return 0.0; }
#endif
};

struct Spot_light {
   float3 positionWS;
   float inv_range_sq;
   float3 directionWS;
   float cone_outer_param;
   float cone_inner_param;
   float3 color;

#if SP_USE_PROJECTED_TEXTURE
   bool use_projected_texture() { return _use_projected_texture; }

   bool _use_projected_texture;
#else
   bool use_projected_texture() { return false; }
#endif

#if SP_HAS_STENCIL_SHADOW_FACTOR
   float stencil_shadow_factor() { return _stencil_shadow_factor; }

   float _stencil_shadow_factor;
#else
   float stencil_shadow_factor() { return 0.0; }
#endif
};

#if SP_USE_ADVANCED_LIGHTING

struct Lights_context {
   bool directional_lights_end()
   {
      return directional_light_index >= light_directional_count;
   }

   Directional_light next_directional_light()
   {
      Directional_light light;

      light.directionWS = light_directional_dir(directional_light_index);
      light.color = light_directional_color(directional_light_index).rgb;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = directional_light_index == 0 ? light_proj_selector.x > 0 : false;
#     endif

#     if SP_HAS_SUN_SHADOW_MAP
         light._use_sun_shadow_map = directional_light_index == 0 && directional_light_0_has_shadow;
#     endif

      directional_light_index += 1;

      return light;
   }
 

   bool point_lights_end()
   {
      return point_light_index >= cluster.point_lights_end;
   }

   Point_light next_point_light()
   {
      light::Cluster_point_light cluster_light = light::light_clusters_point_lights[light::light_clusters_lists[point_light_index]];

      Point_light light;

      light.positionWS = cluster_light.positionWS;
      light.inv_range_sq = cluster_light.inv_range_sq;
      light.color = cluster_light.color;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = false;
#     endif

      point_light_index += 1;

      return light;
   }

   bool spot_lights_end()
   {
      return spot_light_index >= cluster.spot_lights_end;
   }

   Spot_light next_spot_light()
   {
      light::Cluster_spot_light cluster_light = light::light_clusters_spot_lights[light::light_clusters_lists[spot_light_index]];

      Spot_light light;

      light.positionWS = cluster_light.positionWS;
      light.inv_range_sq = cluster_light.inv_range_sq;
      light.directionWS = cluster_light.directionWS;
      light.cone_outer_param = cluster_light.cone_outer_param;
      light.cone_inner_param = cluster_light.cone_inner_param;
      light.color = cluster_light.color;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = false;
#     endif

      spot_light_index += 1;

      return light;
   }

   light::Cluster_index cluster;

   uint directional_light_index;
   uint point_light_index;
   uint spot_light_index;
};

Lights_context acquire_lights_context(float3 positionWS, float4 positionSS)
{
   Lights_context context;

   context.cluster = light::load_cluster(positionWS, positionSS);
   context.directional_light_index = 0;
   context.point_light_index = context.cluster.point_lights_start;
   context.spot_light_index = context.cluster.spot_lights_start;

   return context;
}

#else // SP_USE_ADVANCED_LIGHTING

struct Lights_context {
   bool directional_lights_end()
   {
      return directional_light_index >= light_directional_count;
   }

   Directional_light next_directional_light()
   {
      Directional_light light;

      light.directionWS = light_directional_dir(directional_light_index);
      light.color = light_directional_color(directional_light_index).rgb;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = directional_light_index == 0 ? light_proj_selector.x > 0 : false;
#     endif

#     if SP_HAS_STENCIL_SHADOW_FACTOR      
         light._stencil_shadow_factor = light_directional_color(directional_light_index).w;
#     endif

      directional_light_index += 1;

      return light;
   }
 

   bool point_lights_end()
   {
      return point_light_index >= light_active_point_count;
   }

   Point_light next_point_light()
   {
      Point_light light;

      light.positionWS = light_point_pos(point_light_index);
      light.inv_range_sq = light_point_inv_range_sqr(point_light_index);
      light.color = light_point_color(point_light_index).rgb;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = point_light_index == 0 ? light_proj_selector.y > 0 : false;
#     endif

#     if SP_HAS_STENCIL_SHADOW_FACTOR      
         light._stencil_shadow_factor = light_point_color(point_light_index).w;
#     endif

      point_light_index += 1;

      return light;
   }

   bool spot_lights_end()
   {
      return spot_light;
   }

   Spot_light next_spot_light()
   {
      Spot_light light;

      light.positionWS = light_spot_pos;
      light.inv_range_sq = light_spot_inv_range_sqr;
      light.directionWS = light_spot_dir.xyz;
      light.cone_outer_param = light_spot_params.x;
      light.cone_inner_param = light_spot_params.z;
      light.color = light_spot_color.rgb;

#     if SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = light_proj_selector.z > 0;
#     endif

#     if SP_HAS_STENCIL_SHADOW_FACTOR      
         light._stencil_shadow_factor = light_spot_color.w;
#     endif

      spot_light = true;

      return light;
   }

   uint directional_light_index;
   uint point_light_index;
   bool spot_light;
};

Lights_context acquire_lights_context(float3 positionWS, float4 positionSS)
{
   Lights_context context;

   context.directional_light_index = 0;
   context.point_light_index = 0;
   context.spot_light = !light_active_spot;

   return context;
}

#endif // !SP_USE_ADVANCED_LIGHTING

#endif // LIGHTS_INCLUDED