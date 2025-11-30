#ifndef NORMAL_EXT_COMMON_INCLUDED
#define NORMAL_EXT_COMMON_INCLUDED

#include "constants_list.hlsl"
#include "lighting_utilities.hlsl"
#include "lighting_blinnphong_ext.hlsl"
#include "pixel_utilities.hlsl"

// clang-format off

const static bool normal_ext_use_shadow_map = NORMAL_EXT_USE_SHADOW_MAP;
const static bool normal_ext_use_projected_texture = NORMAL_EXT_USE_PROJECTED_TEXTURE;

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

#ifdef SP_USE_ADVANCED_LIGHTING

float3 do_lighting_diffuse(Normal_ext_lighting_input input, Texture2D<float3> projected_light_texture)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;

   float3 lighting = 0.0;
   lighting = (light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao;
   
   [branch]
   if (light_active) {
      // Directional Light 0
      {
         float intensity = light::intensity_directional(normalWS, light_directional_dir(0));

         [branch]
         if (directional_light_0_has_shadow) {
            intensity *= sample_cascaded_shadow_map(directional_light0_shadow_map, positionWS, 
                                                    directional_light_0_shadow_texel_size,
                                                    directional_light_0_shadow_bias, 
                                                    directional_light0_shadow_matrices);
         }

         float3 light_directional_color_0 = light_directional_color(0).rgb;
         
         if (normal_ext_use_projected_texture && light_proj_selector.x > 0) {
            const float4 positionLS = mul(float4(input.positionWS, 1.0), light_proj_matrix);

            light_directional_color_0 *= sample_projected_light(projected_light_texture, positionLS);
         }

         lighting += intensity * light_directional_color_0;
      }

      lighting += light::intensity_directional(normalWS, light_directional_dir(1)) * light_directional_color(1).rgb;

      const light::Cluster_index cluster = light::load_cluster(positionWS, input.positionSS);
   
      for (uint i = cluster.point_lights_start; i < cluster.point_lights_end; ++i) {
         light::Point_light point_light = light::light_clusters_point_lights[light::light_clusters_lists[i]];
         
         const float intensity = light::intensity_point(normalWS, positionWS, point_light.positionWS,
                                                        point_light.inv_range_sq);

         lighting += intensity * point_light.color;
      }

      for (uint i = cluster.spot_lights_start; i < cluster.spot_lights_end; ++i) {
         light::Spot_light spot_light = light::light_clusters_spot_lights[light::light_clusters_lists[i]];
         
         const float intensity = light::intensity_spot(normalWS, positionWS, spot_light.positionWS,
                                                       spot_light.inv_range_sq, spot_light.directionWS, 
                                                       spot_light.cone_outer_param, spot_light.cone_inner_param);

         lighting += intensity * spot_light.color;
      }
      
      float3 color = lighting * input.diffuse_color;

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

   float3 diffuse_light = (light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao;
   float3 specular_light = 0.0;

   [branch]
   if (light_active) {
      // Directional Light 0
      {
         float3 diffuse_light_contrib = 0.0;
         float3 specular_light_contrib = 0.0;

         light::blinnphong::calculate(diffuse_light_contrib, specular_light_contrib, normalWS, viewWS,
                                      -light_directional_dir(0).xyz, 1.0, 1.0, input.specular_exponent);

         [branch]
         if (directional_light_0_has_shadow) {
            const float shadowing =  sample_cascaded_shadow_map(directional_light0_shadow_map, positionWS, 
                                                                directional_light_0_shadow_texel_size,
                                                                directional_light_0_shadow_bias, 
                                                                directional_light0_shadow_matrices);

            diffuse_light_contrib *= shadowing;
            specular_light_contrib *= shadowing;                                 
         }


         float3 light_directional_color_0 = light_directional_color(0).rgb;
         
         if (normal_ext_use_projected_texture && light_proj_selector.x > 0) {
            const float4 positionLS = mul(float4(input.positionWS, 1.0), light_proj_matrix);

            light_directional_color_0 *= sample_projected_light(projected_light_texture, positionLS);
         }

         diffuse_light += diffuse_light_contrib * light_directional_color_0;
         specular_light += specular_light_contrib * light_directional_color_0;
      }

      light::blinnphong::calculate(diffuse_light, specular_light, normalWS, input.viewWS,
                                   -light_directional_dir(1).xyz, 1.0, light_directional_color(1).rgb, 
                                   input.specular_exponent);

      const light::Cluster_index cluster = light::load_cluster(positionWS, input.positionSS);
   
      for (uint i = cluster.point_lights_start; i < cluster.point_lights_end; ++i) {
         light::Point_light point_light = light::light_clusters_point_lights[light::light_clusters_lists[i]];
         
         light::blinnphong::calculate_point(diffuse_light, specular_light, input.normalWS,
                                            input.positionWS, input.viewWS, point_light.positionWS, 
                                            point_light.inv_range_sq, point_light.color, input.specular_exponent);
      }

      for (uint i = cluster.spot_lights_start; i < cluster.spot_lights_end; ++i) {
         light::Spot_light spot_light = light::light_clusters_spot_lights[light::light_clusters_lists[i]];
         
         light::blinnphong::calculate_spot(diffuse_light, specular_light, input.normalWS,
                                           input.positionWS, input.viewWS, spot_light.positionWS, 
                                           spot_light.inv_range_sq, spot_light.directionWS, 
                                           spot_light.cone_outer_param, spot_light.cone_inner_param,
                                           spot_light.color, input.specular_exponent);
      }

      float3 color = 
         (diffuse_light * input.diffuse_color) + (specular_light * input.specular_color);

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

   float3 diffuse_light = (light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao;
   float3 specular_light = 0.0;

   [branch]
   if (light_active) {
      // Directional Light 0
      {
         float3 diffuse_light_contrib = 0.0;
         float3 specular_light_contrib = 0.0;

         light::blinnphong_normalized::calculate(diffuse_light_contrib, specular_light_contrib, normalWS, viewWS,
                                                 -light_directional_dir(0).xyz, 1.0, 1.0, input.specular_exponent);

         [branch]
         if (directional_light_0_has_shadow) {
            const float shadowing =  sample_cascaded_shadow_map(directional_light0_shadow_map, positionWS, 
                                                                directional_light_0_shadow_texel_size,
                                                                directional_light_0_shadow_bias, 
                                                                directional_light0_shadow_matrices);

            diffuse_light_contrib *= shadowing;
            specular_light_contrib *= shadowing;                                 
         }


         float3 light_directional_color_0 = light_directional_color(0).rgb;
         
         if (normal_ext_use_projected_texture && light_proj_selector.x > 0) {
            const float4 positionLS = mul(float4(input.positionWS, 1.0), light_proj_matrix);

            light_directional_color_0 *= sample_projected_light(projected_light_texture, positionLS);
         }

         diffuse_light += diffuse_light_contrib * light_directional_color_0;
         specular_light += specular_light_contrib * light_directional_color_0;
      }

      light::blinnphong_normalized::calculate(diffuse_light, specular_light, normalWS, input.viewWS,
                                              -light_directional_dir(1).xyz, 1.0, light_directional_color(1).rgb, 
                                              input.specular_exponent);

      const light::Cluster_index cluster = light::load_cluster(positionWS, input.positionSS);
   
      for (uint i = cluster.point_lights_start; i < cluster.point_lights_end; ++i) {
         light::Point_light point_light = light::light_clusters_point_lights[light::light_clusters_lists[i]];
         
         light::blinnphong_normalized::calculate_point(diffuse_light, specular_light, input.normalWS,
                                                       input.positionWS, input.viewWS, point_light.positionWS, 
                                                       point_light.inv_range_sq, point_light.color, input.specular_exponent);
      }

      for (uint i = cluster.spot_lights_start; i < cluster.spot_lights_end; ++i) {
         light::Spot_light spot_light = light::light_clusters_spot_lights[light::light_clusters_lists[i]];
         
         light::blinnphong_normalized::calculate_spot(diffuse_light, specular_light, input.normalWS,
                                                      input.positionWS, input.viewWS, spot_light.positionWS, 
                                                      spot_light.inv_range_sq, spot_light.directionWS, 
                                                      spot_light.cone_outer_param, spot_light.cone_inner_param,
                                                      spot_light.color, input.specular_exponent);
      }

      float3 color = 
         (diffuse_light * input.diffuse_color) + (specular_light * input.specular_color);

      color *= lighting_scale;

      return color;
   }
   else {
      return input.diffuse_color * lighting_scale;
   }
}

#else

float3 do_lighting_diffuse(Normal_ext_lighting_input input, Texture2D<float3> projected_light_texture)
{
   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao;
   
   [branch]
   if (light_active) {
      float3 proj_intensities = 0.0;
      float intensity;
      
      [loop]
      for (uint i = 0; i < 2; ++i) {
         intensity = light::intensity_directional(input.normalWS, light_directional_dir(i));
         diffuse_lighting += intensity * light_directional_color(i);

         if (i == 0) proj_intensities[0] = intensity;
      }
      
      [loop]
      for (uint i = 0; i < light_active_point_count; ++i) {
         intensity = light::intensity_point(input.normalWS, input.positionWS, light_point_pos(i), light_point_inv_range_sqr(i));
         diffuse_lighting += intensity * light_point_color(i);

         if (i == 0) proj_intensities[1] = intensity;
      }

      [branch]
      if (light_active_spot) {
         intensity = light::intensity_spot(input.normalWS, input.positionWS);
         diffuse_lighting += intensity * light_spot_color;

         proj_intensities[2] = intensity;
      }

      if (normal_ext_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(input.positionWS, 1.0), light_proj_matrix));

         const float proj_intensity = dot(light_proj_selector.xyz, proj_intensities);
         diffuse_lighting.rgb -= light_proj_color.rgb * proj_intensity;
         diffuse_lighting.rgb += (light_proj_color.rgb * light_texture) * proj_intensity;
      }

      if (normal_ext_use_shadow_map) {
         const float shadow_mask = saturate(diffuse_lighting.a);

         diffuse_lighting.rgb *= (1.0 - (shadow_mask * (1.0 - input.shadow)));
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
   float4 diffuse_lighting = {(light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      float3 proj_intensities_diffuse = 0.0;
      float3 proj_intensities_specular = 0.0;
      float proj_diffuse_intensity;
      float proj_specular_intensity;

      [loop]
      for (uint i = 0; i < 2; ++i) {
         light::blinnphong::calculate(diffuse_lighting, specular_lighting, input.normalWS, input.viewWS,
                                      -light_directional_dir(i).xyz, 1.0, light_directional_color(i), input.specular_exponent,
                                      proj_diffuse_intensity, proj_specular_intensity);

         if (i == 0) {
            proj_intensities_diffuse[0] = proj_diffuse_intensity;
            proj_intensities_specular[0] = proj_specular_intensity;
         }
      }

      [loop]
      for (uint i = 0; i < light_active_point_count; ++i) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, input.normalWS,
                                            input.positionWS, input.viewWS, light_point_pos(i), 
                                            light_point_inv_range_sqr(i), light_point_color(i), input.specular_exponent,
                                            proj_intensities_diffuse[1], proj_intensities_specular[1]);

         if (i == 0) {
            proj_intensities_diffuse[1] = proj_diffuse_intensity;
            proj_intensities_specular[1] = proj_specular_intensity;
         }
      }

      [branch]
      if (light_active_spot) {
         light::blinnphong::calculate_spot(diffuse_lighting, specular_lighting, input.normalWS,
                                           input.positionWS, input.viewWS, input.specular_exponent,
                                           proj_intensities_diffuse[2], proj_intensities_specular[2]);
      }

      if (normal_ext_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(input.positionWS, 1.0), light_proj_matrix));

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
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - input.shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - input.shadow))));
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
   float4 diffuse_lighting = {(light::ambient(input.normalWS) + input.static_diffuse_lighting) * input.ao, 0.0};
   float4 specular_lighting = 0.0;

   [branch]
   if (light_active) {
      float3 proj_intensities_diffuse = 0.0;
      float3 proj_intensities_specular = 0.0;
      float proj_diffuse_intensity;
      float proj_specular_intensity;

      [loop]
      for (uint i = 0; i < 2; ++i) {
         light::blinnphong_normalized::calculate(diffuse_lighting, specular_lighting, input.normalWS, input.viewWS,
                                                 -light_directional_dir(i).xyz, 1.0, light_directional_color(i), input.specular_exponent,
                                                 proj_diffuse_intensity, proj_specular_intensity);

         if (i == 0) {
            proj_intensities_diffuse[0] = proj_diffuse_intensity;
            proj_intensities_specular[0] = proj_specular_intensity;
         }
      }

      [loop]
      for (uint i = 0; i < light_active_point_count; ++i) {
         light::blinnphong_normalized::calculate_point(diffuse_lighting, specular_lighting, input.normalWS,
                                                       input.positionWS, input.viewWS, light_point_pos(i), 
                                                       light_point_inv_range_sqr(i), light_point_color(i), input.specular_exponent,
                                                       proj_intensities_diffuse[1], proj_intensities_specular[1]);

         if (i == 0) {
            proj_intensities_diffuse[1] = proj_diffuse_intensity;
            proj_intensities_specular[1] = proj_specular_intensity;
         }
      }

      [branch]
      if (light_active_spot) {
         light::blinnphong_normalized::calculate_spot(diffuse_lighting, specular_lighting, input.normalWS,
                                                      input.positionWS, input.viewWS, input.specular_exponent,
                                                      proj_intensities_diffuse[2], proj_intensities_specular[2]);
      }

      if (normal_ext_use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(input.positionWS, 1.0), light_proj_matrix));

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
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - input.shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - input.shadow))));
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

#endif
