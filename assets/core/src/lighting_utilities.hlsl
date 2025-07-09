#ifndef LIGHTING_UTILS_INCLUDED
#define LIGHTING_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "shadow_map_utilities.hlsl"

#pragma warning(disable : 3078) // **sigh**...: loop control variable conflicts with a previous declaration in the outer scope

struct Lighting_input {
   float3 normalWS;
   float3 positionWS;

   float3 static_diffuse_lighting;
   
   float ambient_occlusion;

   bool use_projected_light;
   float3 projected_light_texture_color;

   bool use_shadow;
   float shadow;

   bool detailing_pass;
   float detailing_pass_intensity;
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

void spot_params(float3 positionWS, out float attenuation, out float3 light_dirWS)
{
   const float3 unnormalized_light_vectorWS = light_spot_pos.xyz - positionWS;

   light_dirWS = normalize(unnormalized_light_vectorWS);

   const float outer_cone = light_spot_params.x;

   const float theta = max(dot(-light_dirWS, light_spot_dir.xyz), 0.0);
   const float cone_falloff = saturate((theta - outer_cone) * light_spot_params.z);

   attenuation =
      attenuation_point(unnormalized_light_vectorWS, light_spot_inv_range_sqr);
   attenuation *= cone_falloff;
}

float intensity_directional(float3 normalWS, float3 direction)
{
   return max(dot(normalWS, -direction), 0.0);
}

float intensity_point(float3 normal, float3 position, float3 light_position,
                      float inv_range_sqr)
{
   const float3 unnormalized_light_vector = light_position - position;
   const float3 light_dir = normalize(unnormalized_light_vector);
   const float intensity = saturate(dot(normal, light_dir));
   const float attenuation =
      attenuation_point(unnormalized_light_vector, inv_range_sqr);

   return attenuation * intensity;
}

float intensity_spot(float3 normalWS, float3 positionWS)
{
   float3 light_dirWS;
   float attenuation;

   spot_params(positionWS, attenuation, light_dirWS);

   const float intensity = saturate(dot(normalWS, light_dirWS));

   return intensity * attenuation;
}

float3 calculate_normal(Lighting_input input)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;

   float4 light = float4((ambient(normalWS) + input.static_diffuse_lighting) * input.ambient_occlusion, 0.0);

   // clang-format off

   [branch]
   if (light_active) { 
      float3 proj_light_intensities = light.rgb;

      [loop]
      for (uint i = 0; i < 2; ++i) {
         const float intensity = intensity_directional(normalWS, light_directional_dir(i));
         light += intensity * light_directional_color(i);

         if (i == 0) proj_light_intensities[0] = intensity;
      }

      [loop]
      for (uint i = 0; i < light_active_point_count; ++i) {
         const float intensity = intensity_point(normalWS, positionWS, light_point_pos(i),
                                                 light_point_inv_range_sqr(i));
         light += intensity * light_point_color(i);

         if (i == 0) proj_light_intensities[1] = intensity;
      }

      if (light_active_spot) {
         const float intensity = intensity_spot(normalWS, positionWS);
         light += intensity * light_spot_color;

         proj_light_intensities[2] = intensity;
      }

      if (input.use_projected_light) {
         const float proj_light_intensity = dot(light_proj_selector.xyz, proj_light_intensities);

         light.rgb -= (light_proj_color.rgb * proj_light_intensity);
         light.rgb += (input.projected_light_texture_color * light_proj_color.rgb *
                       proj_light_intensity);
      }

      if (limit_normal_shader_bright_lights) {
         const float scale = max(max(max(light.r, light.g), light.b), 1.0);
         light.rgb *= rcp(scale);
      }

      light.rgb *= lighting_scale;
   }
   else {
      light = float4(lighting_scale.xxx, 0.0);
   }

   if (input.detailing_pass) light.rgb = input.detailing_pass_intensity;
         
   if (input.use_shadow) {
      const float shadow = 1.0 - (light.a * (1.0 - input.shadow));

      light.rgb *= shadow;
   }

   // clang-format on

   return light.rgb;
}

float3 calculate_advanced(Lighting_input input)
{
   const float3 normalWS = input.normalWS;
   const float3 positionWS = input.positionWS;

   float4 light = float4((ambient(normalWS) + input.static_diffuse_lighting) * input.ambient_occlusion, 0.0);

   // clang-format off

   [branch]
   if (light_active) { 
         float3 proj_light_intensities = light.rgb;

         // Directional Light 0
         {
            float intensity = intensity_directional(normalWS, light_directional_dir(0));

            [branch]
            if (directional_light_0_has_shadow) {
               intensity *= sample_cascaded_shadow_map(directional_light0_shadow_map, positionWS, 
                                                       directional_light_0_shadow_texel_size,
                                                       directional_light_0_shadow_bias, 
                                                       directional_light0_shadow_matrices);
            }

            light += intensity * light_directional_color(0);

            proj_light_intensities[0] = intensity;
         }

         light += intensity_directional(normalWS, light_directional_dir(1)) * light_directional_color(1);

         [loop]
         for (uint i = 0; i < light_active_point_count; ++i) {
            const float intensity = intensity_point(normalWS, positionWS, light_point_pos(i),
                                                    light_point_inv_range_sqr(i));
            light += intensity * light_point_color(i);

            if (i == 0) proj_light_intensities[1] = intensity;
         }

         if (light_active_spot) {
            const float intensity = intensity_spot(normalWS, positionWS);
            light += intensity * light_spot_color;

            proj_light_intensities[2] = intensity;
         }

         if (input.use_projected_light) {
            const float proj_light_intensity = dot(light_proj_selector.xyz, proj_light_intensities);

            light.rgb -= (light_proj_color.rgb * proj_light_intensity);
            light.rgb += (input.projected_light_texture_color * light_proj_color.rgb *
                          proj_light_intensity);
         }

         light.rgb *= lighting_scale;
   }
   else {
      light = float4(lighting_scale.xxx, 0.0);
   }

   if (input.detailing_pass) light.rgb = input.detailing_pass_intensity;

   // clang-format on

   return light.rgb;
}

float3 calculate(Lighting_input input)
{
#  ifdef SP_USE_ADVANCED_LIGHTING
      return calculate_advanced(input);
#  else
      return calculate_normal(input);
#  endif
}

namespace blinnphong {

void calculate(inout float4 in_out_diffuse, inout float4 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float4 light_color, float exponent,
               out float out_diffuse_intensity, out float out_specular_intensity)
{
   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float specular = pow(NdotH, exponent) * attenuation;
   const float diffuse = saturate(dot(N, L)) * attenuation;

   out_diffuse_intensity = diffuse;
   out_specular_intensity = specular;

   in_out_diffuse += (diffuse * light_color);
   in_out_specular += (specular * light_color);
}

void calculate_point(inout float4 in_out_diffuse, inout float4 in_out_specular,
                     float3 normal, float3 position, float3 view_normal,
                     float3 light_position, float inv_range_sqr, float4 light_color,
                     float exponent, out float out_diffuse_intensity,
                     out float out_specular_intensity)
{
   const float3 unnormalized_light_vector = light_position - position;
   const float3 light_dir = normalize(unnormalized_light_vector);
   const float attenuation =
      attenuation_point(unnormalized_light_vector, inv_range_sqr);

   calculate(in_out_diffuse, in_out_specular, normal, view_normal, light_dir,
             attenuation, light_color, exponent, out_diffuse_intensity,
             out_specular_intensity);
}

void calculate_spot(inout float4 in_out_diffuse, inout float4 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 view_normalWS,
                    float exponent, out float out_diffuse_intensity,
                    out float out_specular_intensity)
{
   float3 light_dirWS;
   float attenuation;
   spot_params(positionWS, attenuation, light_dirWS);

   calculate(in_out_diffuse, in_out_specular, normalWS, view_normalWS,
             light_dirWS, attenuation, light_spot_color, exponent,
             out_diffuse_intensity, out_specular_intensity);
}

void calculate(inout float4 in_out_diffuse, inout float4 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float4 light_color, float exponent)
{
   float2 ignore;

   calculate(in_out_diffuse, in_out_specular, N, V, L, attenuation, light_color,
             exponent, ignore.x, ignore.y);
}

void calculate_point(inout float4 in_out_diffuse, inout float4 in_out_specular,
                     float3 normal, float3 position, float3 view_normal,
                     float3 light_position, float inv_range_sqr,
                     float4 light_color, float exponent)
{

   float2 ignore;

   calculate_point(in_out_diffuse, in_out_specular, normal, position,
                   view_normal, light_position, inv_range_sqr, light_color,
                   exponent, ignore.x, ignore.y);
}

void calculate_spot(inout float4 in_out_diffuse, inout float4 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 view_normalWS,
                    float exponent)
{

   float2 ignore;

   calculate_spot(in_out_diffuse, in_out_specular, normalWS, positionWS,
                  view_normalWS, exponent, ignore.x, ignore.y);
}
}
}

#endif