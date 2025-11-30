#if 0

#ifndef LIGHTS_INCLUDED
#define LIGHTS_INCLUDED

struct Point_light {
   float3 positionWS;
   float inv_range_sq;
   float3 color;

#ifdef SP_USE_PROJECTED_TEXTURE
   bool use_projected_texture() { return _use_texture; }

   bool _use_projected_texture;
#else
   bool use_projected_texture() { return false; }
#endif

#ifdef SP_USE_STENCIL_SHADOW_MAP
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

#ifdef SP_USE_PROJECTED_TEXTURE
   bool use_projected_texture() { return _use_texture; }

   bool _use_projected_texture;
#else
   bool use_projected_texture() { return false; }
#endif

#ifdef SP_USE_STENCIL_SHADOW_MAP
   float stencil_shadow_factor() { return _stencil_shadow_factor; }

   float _stencil_shadow_factor;
#else
   float stencil_shadow_factor() { return 0.0; }
#endif
};

#ifdef SP_USE_ADVANCED_LIGHTING

struct Lights_context {
   bool point_lights();

   Point_light next_point_light();

   bool spot_lights();

   Spot_light next_spot_lights();
};

Lights_context acquire_lights_context(float3 positionWS, float4 positionSS)
{

}

#else

struct Lights_context {
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

#     ifdef SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = point_light_index == 0 ? light_proj_selector.y > 0 : false;
#     endif

#     ifdef SP_USE_STENCIL_SHADOW_MAP      
         light._stencil_shadow_factor = light_point_color(point_light_index).w;
#     endif

      point_light_index += 1;

      return light;
   }

   bool spot_lights_end()
   {
      return spot_light;
   }

   Spot_light next_spot_lights()
   {
      Spot_light light;

      light.positionWS = light_spot_pos;
      light.inv_range_sq = light_spot_inv_range_sqr;
      light.directionWS = light_spot_dir.xyz;
      light.cone_outer_param = light_spot_params.x;
      light.cone_inner_param = light_spot_params.z;
      light.color = light_spot_color.rgb;

#     ifdef SP_USE_PROJECTED_TEXTURE      
         light._use_projected_texture = point_light_index == 0 ? light_proj_selector.z > 0 : false;
#     endif

#     ifdef SP_USE_STENCIL_SHADOW_MAP      
         light._stencil_shadow_factor = light_spot_color.w;
#     endif

      spot_light = true;

      return light;
   }

   uint point_light_index;
   bool spot_light;
};

Lights_context acquire_lights_context(float3 positionWS, float4 positionSS)
{
   Lights_context context;

   context.point_light_index = 0;
   context.spot_light = !light_active_spot;

   return context;
}

#endif

#endif