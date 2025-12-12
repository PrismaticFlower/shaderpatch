#ifndef LIGHTING_UTILS_INCLUDED
#define LIGHTING_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "lights.hlsli"

#pragma warning(disable : 3078) // **sigh**...: loop control variable conflicts with a previous declaration in the outer scope

struct Lighting {
   float3 color;
   float intensity;
};

namespace light {

float specular_exp_to_roughness(float exponent)
{
   return sqrt(max(2.0 / (exponent + 2.0), 0.0));
}

float3 ambient(float3 normalWS)
{
   float factor = normalWS.y * -0.5 + 0.5;

   float3 color;

   color.rgb = light_ambient_color_top.rgb * -factor + light_ambient_color_top.rgb;
   color.rgb = light_ambient_color_bottom.rgb * factor + color.rgb;

   return color;
}

float attenuation_point(float3 unnormalized_light_vector, float inv_range_sqr)
{
   const float dist_sqr = dot(unnormalized_light_vector, unnormalized_light_vector);
   const float attenuation = 1.0 - saturate(dist_sqr * inv_range_sqr);

   return attenuation;
}

float intensity_directional(float3 normalWS, float3 direction)
{
   return max(dot(normalWS, -direction), 0.0);
}

float intensity_point(float3 normalWS, float3 positionWS, Point_light light)
{
   const float3 unnormalized_light_vectorWS = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);
   const float intensity = saturate(dot(normalWS, light_dirWS));
   const float attenuation = attenuation_point(unnormalized_light_vectorWS, light.inv_range_sq);

   return attenuation * intensity;
}

float intensity_spot(float3 normalWS, float3 positionWS, Spot_light light)
{
   const float3 unnormalized_light_vectorWS = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);

   const float outer_cone = light_spot_params.x;
   
   const float theta = max(dot(-light_dirWS, light.directionWS), 0.0);
   const float cone_falloff = saturate((theta - light.cone_outer_param) * light.cone_inner_param);

   const float attenuation = attenuation_point(unnormalized_light_vectorWS, light.inv_range_sq);
   const float intensity = saturate(dot(normalWS, light_dirWS));

   return intensity * attenuation * cone_falloff;
}

Lighting calculate(float3 normalWS, float3 positionWS,
                   float3 static_diffuse_lighting, float ambient_occlusion,
                   bool use_projected_light = false, float3 projected_light_texture_color = 0.0)
{
   Lighting lighting;

   float4 light = float4((ambient(normalWS) + static_diffuse_lighting) * ambient_occlusion, 0.0);

   // clang-format off

   [branch]
   if (light_active) { 
      Lights_context context = acquire_lights_context();

      while (!context.directional_lights_end()) {
         Directional_light directional_light = context.next_directional_light();

         const float intensity = intensity_directional(normalWS, directional_light.directionWS);
         float3 light_color = directional_light.color;

         if (directional_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light += intensity * float4(light_color, directional_light.stencil_shadow_factor());
      }

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         const float intensity = intensity_point(normalWS, positionWS, point_light);
         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light += intensity * float4(light_color, point_light.stencil_shadow_factor());
      }

      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         const float intensity = intensity_spot(normalWS, positionWS, spot_light);
         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         light += intensity * float4(light_color, spot_light.stencil_shadow_factor());
      }

      if (limit_normal_shader_bright_lights) {
         const float scale = max(max(max(light.r, light.g), light.b), 1.0);
         light.rgb *= rcp(scale);
      }

      light.rgb *= lighting_scale;

      lighting.color = light.rgb;
      lighting.intensity = light.a;
   }
   else
   {
      lighting.color = lighting_scale;
      lighting.intensity = 0.0;
   }

   // clang-format on

   return lighting;
}

namespace blinnphong {

void calculate(inout float4 in_out_diffuse, inout float4 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float3 light_color, float shadow_factor, float exponent)
{
   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float specular = pow(NdotH, exponent) * attenuation;
   const float diffuse = saturate(dot(N, L)) * attenuation;

   in_out_diffuse += (diffuse * float4(light_color, shadow_factor));
   in_out_specular += (specular * float4(light_color, shadow_factor));
}

void calculate_point(inout float4 in_out_diffuse, inout float4 in_out_specular,
                     float3 normalWS, float3 positionWS, float3 viewWS, 
                     Point_light light, float exponent)
{
   const float3 unnormalized_light_vectorWS = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);
   const float attenuation =
      attenuation_point(unnormalized_light_vectorWS, light.inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normalWS, viewWS, light_dirWS,
             attenuation, light.color, light.stencil_shadow_factor(), exponent);
}

void calculate_spot(inout float4 in_out_diffuse, inout float4 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 viewWS,
                    Spot_light light, float exponent)
{
   const float3 unnormalized_light_vectorWS = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);

   const float theta = max(dot(-light_dirWS, light.directionWS), 0.0);
   const float cone_falloff = saturate((theta - light.cone_outer_param) * light.cone_inner_param);

   const float attenuation = attenuation_point(unnormalized_light_vectorWS, light.inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normalWS, viewWS,
             light_dirWS, attenuation * cone_falloff, light.color, 
             light.stencil_shadow_factor(), exponent);
}
}
}

#endif