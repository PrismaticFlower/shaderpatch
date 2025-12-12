#ifndef NORMAL_BF3_COMMON_INCLUDED
#define NORMAL_BF3_COMMON_INCLUDED

#include "constants_list.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float3 base_diffuse_color;
   float  gloss_map_weight;
   float3 base_specular_color;
   float  specular_exponent;
   bool   use_ao_texture;
   bool   use_emissive_texture;
   float  emissive_texture_scale;
   float  emissive_power;
   bool   use_env_map;
   float  env_map_vis;
   float  dynamic_normal_sign;
   bool   use_outline_light;
   float3 outline_light_color;
   float  outline_light_width; 
   float  outline_light_fade;
   float  height_scale;
   float  parallax_scale_x;
};

// clang-format off

const static bool normal_bf3_use_shadow_map = SP_USE_STENCIL_SHADOW_MAP;
const static bool normal_bf3_use_projected_texture = SP_USE_PROJECTED_TEXTURE;

struct surface_info {
   float3 normalWS;
   float3 positionWS;
   float4 positionSS;
   float3 viewWS;
   float3 diffuse_color; 
   float3 static_diffuse_lighting; 
   float3 specular_color; 
   float shadow;
   float ao;
};

float3 do_lighting(surface_info surface, Texture2D<float3> projected_light_texture)
{
   const float3 positionWS = surface.positionWS;
   const float3 normalWS = surface.normalWS;
   const float3 viewWS = surface.viewWS;

   float4 diffuse_lighting = {(light::ambient(normalWS) + surface.static_diffuse_lighting) * surface.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      const float3 projected_light_texture_color = 
         normal_bf3_use_projected_texture ? sample_projected_light(projected_light_texture, 
                                                               mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                          : 0.0;

      Lights_context context = acquire_lights_context(positionWS, surface.positionSS);

      while (!context.directional_lights_end()) {
         Directional_light directional_light = context.next_directional_light();

         float3 light_color = directional_light.color;

         if (directional_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float shadowing = 1.0;

         if (directional_light.use_sun_shadow_map()) {
            shadowing = sample_sun_shadow_map(positionWS);
         }

         light::blinnphong::calculate(diffuse_lighting, specular_lighting, normalWS, viewWS,
                                      -directional_light.directionWS, shadowing, light_color, 
                                      directional_light.stencil_shadow_factor(), specular_exponent);
      }

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, viewWS, point_light, specular_exponent);
      }
      
      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light::blinnphong::calculate_spot(diffuse_lighting, specular_lighting, normalWS,
                                           positionWS, viewWS, spot_light, specular_exponent);
      }

      if (normal_bf3_use_shadow_map) {
         const float shadow_diffuse_mask = saturate(diffuse_lighting.a);
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - surface.shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - surface.shadow))));
      }


      float3 color = 
         (diffuse_lighting.rgb * surface.diffuse_color) + (specular_lighting.rgb * surface.specular_color);

      color *= lighting_scale;

      if (use_outline_light) {
         const float inv_NdotV = 1 - saturate(dot(surface.normalWS, surface.viewWS));
         const float LdotN = saturate(dot(-light_directional_dir(0).xyz, surface.normalWS));
   
         float3 outline_color = smoothstep(1.0 - outline_light_width, 1.0, inv_NdotV) * lerp(LdotN, 1.0, outline_light_fade);
         outline_color *= outline_light_color;

         color *= (0.5 + outline_color) * 2.0;
      }

      return color;
   }
   else {
      return surface.diffuse_color * lighting_scale;
   }
   
   return 0.0;
}


#endif
