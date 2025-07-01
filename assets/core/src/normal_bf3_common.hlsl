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
};

// clang-format off

const static bool normal_bf3_use_shadow_map = NORMAL_BF3_USE_SHADOW_MAP;
const static bool normal_bf3_use_projected_texture = NORMAL_BF3_USE_PROJECTED_TEXTURE;

struct surface_info {
   float3 normalWS;
   float3 positionWS;
   float3 viewWS;
   float3 diffuse_color; 
   float3 static_diffuse_lighting; 
   float3 specular_color; 
   float shadow;
   float ao;
};

float3 do_lighting(surface_info surface, Texture2D<float3> projected_light_texture)
{
   float4 diffuse_lighting = {(light::ambient(surface.normalWS) + surface.static_diffuse_lighting) * surface.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      float3 proj_intensities_diffuse = 0.0;
      float3 proj_intensities_specular = 0.0;
      float proj_diffuse_intensity;
      float proj_specular_intensity;

      [loop]
      for (uint i = 0; i < 2; ++i) {
         light::blinnphong::calculate(diffuse_lighting, specular_lighting, surface.normalWS, surface.viewWS,
                                      -light_directional_dir(i).xyz, 1.0, light_directional_color(i), specular_exponent,
                                      proj_diffuse_intensity, proj_specular_intensity);

         if (i == 0) {
            proj_intensities_diffuse[0] = proj_diffuse_intensity;
            proj_intensities_specular[0] = proj_specular_intensity;
         }
      }

      [loop]
      for (uint i = 0; i < light_active_point_count; ++i) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, surface.normalWS,
                                            surface.positionWS, surface.viewWS, light_point_pos(i), 
                                            light_point_inv_range_sqr(i), light_point_color(i), specular_exponent,
                                            proj_intensities_diffuse[1], proj_intensities_specular[1]);

         if (i == 0) {
            proj_intensities_diffuse[1] = proj_diffuse_intensity;
            proj_intensities_specular[1] = proj_specular_intensity;
         }
      }

      [branch]
      if (light_active_spot) {
         light::blinnphong::calculate_spot(diffuse_lighting, specular_lighting, surface.normalWS,
                                           surface.positionWS, surface.viewWS, specular_exponent,
                                           proj_intensities_diffuse[2], proj_intensities_specular[2]);
      }

      if (normal_bf3_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(surface.positionWS, 1.0), light_proj_matrix));

         const float3 masked_light_color = (light_proj_color.rgb * light_texture);

         const float diffuse_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_diffuse);

         diffuse_lighting.rgb -= (light_proj_color.rgb * diffuse_proj_intensity);
         diffuse_lighting.rgb += masked_light_color * diffuse_proj_intensity;

         const float specular_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_specular);

         specular_lighting.rgb -= light_proj_color.rgb * specular_proj_intensity;
         specular_lighting.rgb += masked_light_color * specular_proj_intensity;
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
}


#endif
