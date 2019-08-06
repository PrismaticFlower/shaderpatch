#ifndef NORMAL_EXT_COMMON_INCLUDED
#define NORMAL_EXT_COMMON_INCLUDED

#include "constants_list.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

const static bool normal_ext_use_shadow_map = NORMAL_EXT_USE_SHADOW_MAP;
const static bool normal_ext_use_projected_texture = NORMAL_EXT_USE_PROJECTED_TEXTURE;

float3 do_lighting_diffuse(float3 normalWS, float3 positionWS, float3 diffuse_color,
                           float3 static_diffuse_lighting, float shadow,
                           float ao, Texture2D<float3> projected_light_texture)
{
   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(normalWS) + static_diffuse_lighting) * ao;

   if (ps_light_active_directional) {
      float3 proj_intensities = 0.0;
      float intensity;

      intensity = light::intensity_directional(normalWS, light_directional_0_dir);
      diffuse_lighting += intensity * light_directional_0_color;

      proj_intensities[0] = intensity;

      diffuse_lighting += light::intensity_directional(normalWS, light_directional_1_dir) * light_directional_1_color;

      if (ps_light_active_point_0) {
         intensity = light::intensity_point(normalWS, positionWS, light_point_0_pos, light_point_0_inv_range_sqr);
         diffuse_lighting += intensity * light_point_0_color;

         proj_intensities[1] = intensity;
      }

      if (ps_light_active_point_1) {
         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_1_pos, light_point_1_inv_range_sqr) * light_point_1_color;
      }

      if (ps_light_active_point_23) {
         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_2_pos, light_point_2_inv_range_sqr) * light_point_2_color;

         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_3_pos, light_point_3_inv_range_sqr) * light_point_3_color;
      }
      else if (ps_light_active_spot_light) {
         intensity = light::intensity_spot(normalWS, positionWS);
         diffuse_lighting += intensity * light_spot_color;

         proj_intensities[2] = intensity;
      }

      if (normal_ext_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(positionWS, 1.0), light_proj_matrix));

         const float proj_intensity = dot(light_proj_selector.xyz, proj_intensities);
         diffuse_lighting.rgb -= light_proj_color.rgb * proj_intensity;
         diffuse_lighting.rgb += (light_proj_color.rgb * light_texture) * proj_intensity;
      }

      if (normal_ext_use_shadow_map) {
         const float shadow_mask = saturate(diffuse_lighting.a);

         diffuse_lighting.rgb *= (1.0 - (shadow_mask * (1.0 - shadow)));
      }

      float3 color = diffuse_lighting.rgb * diffuse_color;

      color *= lighting_scale;

      return color;
   }
   else {
      return diffuse_color * lighting_scale;
   }
}

float3 do_lighting(float3 normalWS, float3 positionWS, float3 view_normalWS,
                   float3 diffuse_color, float3 static_diffuse_lighting, 
                   float3 specular_color, float specular_exponent, float shadow,
                   float ao, Texture2D<float3> projected_light_texture)
{
   float4 diffuse_lighting = {(light::ambient(normalWS) + static_diffuse_lighting) * ao, 0.0};
   float4 specular_lighting = 0.0;

   if (ps_light_active_directional) {
      float3 proj_intensities_diffuse = 0.0;
      float3 proj_intensities_specular = 0.0;

      light::blinnphong::calculate(diffuse_lighting, specular_lighting, normalWS, view_normalWS,
                                   -light_directional_0_dir.xyz, 1.0, light_directional_0_color, specular_exponent,
                                   proj_intensities_diffuse[0], proj_intensities_specular[0]);

      light::blinnphong::calculate(diffuse_lighting, specular_lighting, normalWS, view_normalWS, 
                                   -light_directional_1_dir.xyz, 1.0, light_directional_1_color, specular_exponent);

      if (ps_light_active_point_0) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_0_pos, 
                                            light_point_0_inv_range_sqr, light_point_0_color, specular_exponent,
                                            proj_intensities_diffuse[1], proj_intensities_specular[1]);
      }

      if (ps_light_active_point_1) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_1_pos, 
                                            light_point_1_inv_range_sqr, light_point_1_color,
                                            specular_exponent);
      }

      if (ps_light_active_point_23) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_2_pos, 
                                            light_point_2_inv_range_sqr, light_point_2_color,
                                            specular_exponent);
         
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_3_pos, 
                                            light_point_3_inv_range_sqr, light_point_3_color,
                                            specular_exponent);
      }
      else if (ps_light_active_spot_light) {
         light::blinnphong::calculate_spot(diffuse_lighting, specular_lighting, normalWS,
                                           view_normalWS, positionWS, specular_exponent,
                                           proj_intensities_diffuse[2], proj_intensities_specular[2]);
      }

      if (normal_ext_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(positionWS, 1.0), light_proj_matrix));

         const float3 masked_light_color = (light_proj_color.rgb * light_texture);

         const float diffuse_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_diffuse);

         diffuse_lighting.rgb -= (light_proj_color.rgb * diffuse_proj_intensity);
         diffuse_lighting.rgb += masked_light_color * diffuse_proj_intensity;

         const float specular_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_specular);

         specular_lighting.rgb -= light_proj_color.rgb * specular_proj_intensity;
         specular_lighting.rgb += masked_light_color * specular_proj_intensity;
      }

      if (normal_ext_use_shadow_map) {
         const float shadow_diffuse_mask = saturate(diffuse_lighting.a);
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - shadow))));
      }


      float3 color = 
         (diffuse_lighting.rgb * diffuse_color) + (specular_lighting.rgb * specular_color);

      color *= lighting_scale;

      return color;
   }
   else {
      return diffuse_color * lighting_scale;
   }
}


#endif
