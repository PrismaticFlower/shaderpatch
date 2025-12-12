#ifndef NORMAL_EXT_COMMON_INCLUDED
#define NORMAL_EXT_COMMON_INCLUDED

#include "constants_list.hlsl"
#include "lighting_utilities.hlsl"
#include "lighting_blinnphong_ext.hlsl"
#include "pixel_utilities.hlsl"

// clang-format off

const static bool normal_ext_use_shadow_map = SP_USE_STENCIL_SHADOW_MAP;
const static bool normal_ext_projected_texture = SP_USE_PROJECTED_TEXTURE;

struct Normal_ext_lighting_input 
{
   float3 normalWS;
   float3 positionWS;

   float4 positionSS;

   float3 viewWS;

   float3 diffuse_color;
   float3 static_diffuse_lighting; 

   float3 specular_color;
   float specular_exponent;

   float shadow;
   float ao;
};

float3 do_lighting_diffuse(Normal_ext_lighting_input input, Texture2D<float3> projected_light_texture)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;

   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(normalWS) + input.static_diffuse_lighting) * input.ao;
   
   [branch]
   if (light_active) {
      const float3 projected_light_texture_color = 
         normal_ext_projected_texture ? sample_projected_light(projected_light_texture, 
                                                               mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                      : 0.0;

      Lights_context context = acquire_lights_context(positionWS, input.positionSS);

      while (!context.directional_lights_end()) {
         Directional_light directional_light = context.next_directional_light();

         float intensity = light::intensity_directional(normalWS, directional_light.directionWS);

         if (directional_light.use_sun_shadow_map()) {
            intensity *= sample_sun_shadow_map(positionWS);
         }

         float3 light_color = directional_light.color;

         if (directional_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         diffuse_lighting += intensity * float4(light_color, directional_light.stencil_shadow_factor());
      }

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         const float intensity = light::intensity_point(normalWS, positionWS, point_light);
         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         diffuse_lighting += intensity * float4(light_color, point_light.stencil_shadow_factor());
      }

      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         const float intensity = light::intensity_spot(normalWS, positionWS, spot_light);
         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         diffuse_lighting += intensity * float4(light_color, spot_light.stencil_shadow_factor());
      }

      if (normal_ext_use_shadow_map) {
         const float shadow = input.shadow;
         const float shadow_mask = saturate(diffuse_lighting.a);

         diffuse_lighting.rgb *= (1.0 - (shadow_mask * (1.0 - shadow)));
      }

      float3 color = diffuse_lighting.rgb * input.diffuse_color;

      color *= lighting_scale;

      return color;
   }
   else {
      return input.diffuse_color * lighting_scale;
   }
}

float3 do_lighting(Normal_ext_lighting_input input, Texture2D<float3> projected_light_texture)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;
   const float3 viewWS = input.viewWS;

   const float specular_exponent = input.specular_exponent;

   float4 diffuse_lighting = {(light::ambient(normalWS) + input.static_diffuse_lighting) * input.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      const float3 projected_light_texture_color = 
         normal_ext_projected_texture ? sample_projected_light(projected_light_texture, 
                                                               mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                      : 0.0;

      Lights_context context = acquire_lights_context(positionWS, input.positionSS);

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

      if (normal_ext_use_shadow_map) {
         const float shadow = input.shadow;

         const float shadow_diffuse_mask = saturate(diffuse_lighting.a);
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - shadow))));
      }

      float3 color = 
         (diffuse_lighting.rgb * input.diffuse_color) + (specular_lighting.rgb * input.specular_color);

      color *= lighting_scale;

      return color;
   }
   else {
      return input.diffuse_color * lighting_scale;
   }
}

float3 do_lighting_normalized(Normal_ext_lighting_input input, Texture2D<float3> projected_light_texture)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;
   const float3 viewWS = input.viewWS;

   const float specular_exponent = input.specular_exponent;

   float4 diffuse_lighting = {(light::ambient(normalWS) + input.static_diffuse_lighting) * input.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      const float3 projected_light_texture_color = 
         normal_ext_projected_texture ? sample_projected_light(projected_light_texture, 
                                                               mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                      : 0.0;

      Lights_context context = acquire_lights_context(positionWS, input.positionSS);

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

         light::blinnphong_normalized::calculate(diffuse_lighting, specular_lighting, normalWS, viewWS,
                                                 -directional_light.directionWS, shadowing, light_color, 
                                                 directional_light.stencil_shadow_factor(), specular_exponent);
      }

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light::blinnphong_normalized::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                                       positionWS, viewWS, point_light, specular_exponent);
      }
      
      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light::blinnphong_normalized::calculate_spot(diffuse_lighting, specular_lighting, normalWS,
                                                      positionWS, viewWS, spot_light, specular_exponent);
      }

      if (normal_ext_use_shadow_map) {
         const float shadow = input.shadow;

         const float shadow_diffuse_mask = saturate(diffuse_lighting.a);
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - shadow))));
      }

      float3 color = 
         (diffuse_lighting.rgb * input.diffuse_color) + (specular_lighting.rgb * input.specular_color);

      color *= lighting_scale;

      return color;
   }
   else {
      return input.diffuse_color * lighting_scale;
   }
}

#endif
