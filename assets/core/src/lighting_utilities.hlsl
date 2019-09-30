#ifndef LIGHTING_UTILS_INCLUDED
#define LIGHTING_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"

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

float intensity_directional(float3 normalWS, float4 direction)
{
   return max(dot(normalWS, -direction.xyz), 0.0);
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

Lighting calculate(float3 normalWS, float3 positionWS,
                   float3 static_diffuse_lighting, bool use_projected_light = false,
                   float3 projected_light_texture_color = 0.0)
{
   Lighting lighting;

   float4 light = float4(ambient(normalWS) + static_diffuse_lighting, 0.0);

   if (ps_light_active_directional) {
      float4 intensity = float4(light.rgb, 1.0);

      intensity.x = intensity_directional(normalWS, light_directional_0_dir);
      light += intensity.x * light_directional_0_color;

      intensity.w = intensity_directional(normalWS, light_directional_1_dir);
      light += intensity.w * light_directional_1_color;

      if (ps_light_active_point_0) {
         intensity.y = intensity_point(normalWS, positionWS, light_point_0_pos,
                                       light_point_0_inv_range_sqr);
         light += intensity.y * light_point_0_color;
      }

      if (ps_light_active_point_1) {
         intensity.w = intensity_point(normalWS, positionWS, light_point_1_pos,
                                       light_point_1_inv_range_sqr);
         light += intensity.w * light_point_1_color;
      }

      if (ps_light_active_point_23) {
         intensity.w = intensity_point(normalWS, positionWS, light_point_2_pos,
                                       light_point_2_inv_range_sqr);
         light += intensity.w * light_point_2_color;

         intensity.w = intensity_point(normalWS, positionWS, light_point_3_pos,
                                       light_point_3_inv_range_sqr);
         light += intensity.w * light_point_3_color;
      }
      else if (ps_light_active_spot_light) {
         intensity.z = intensity_spot(normalWS, positionWS);
         light += intensity.z * light_spot_color;
      }

      if (use_projected_light) {
         const float proj_light_intensity = dot(light_proj_selector, intensity);

         light.rgb -= (light_proj_color.rgb * proj_light_intensity);
         light.rgb += (projected_light_texture_color * light_proj_color.rgb *
                       proj_light_intensity);
      }

      if (limit_normal_shader_bright_lights) {
         const float scale = max(max(max(light.r, light.g), light.b), 1.0);
         light.rgb *= rcp(scale);
      }

      light.rgb *= lighting_scale;

      lighting.color = light.rgb;
      lighting.intensity = light.a;
   }
   else {
      lighting.color = lighting_scale;
      lighting.intensity = 0.0;
   }

   return lighting;
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

namespace pbr {

static const float PI = 3.14159265359;
static const float min_roughness = 0.025;

float specular_occlusion(float NdotV, float ao, float roughness)
{
   return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

float attenuation_point_smooth(float distance_sqr, float inv_range_sqr)
{
   const float factor = distance_sqr * inv_range_sqr;
   const float smooth_factor = saturate(1.0 - factor * factor);

   return smooth_factor * smooth_factor;
}

float attenuation_point(float3 unnormalized_light_vector, float inv_range_sqr)
{
   const float dist_sqr = dot(unnormalized_light_vector, unnormalized_light_vector);
   const float attenuation = 1.0 / max(dist_sqr, 0.01 * 0.01);

   return attenuation * attenuation_point_smooth(dist_sqr, inv_range_sqr);
}

void point_params(float3 position, float3 light_position, float inv_range_sqr,
                  out float3 light_dir, out float light_attenuation)
{
   const float3 unnormalized_light_vector = light_position - position;

   light_dir = normalize(unnormalized_light_vector);
   light_attenuation = attenuation_point(unnormalized_light_vector, inv_range_sqr);
}

void spot_params(float3 positionWS, out float attenuation, out float3 light_dirWS)
{
   float dist_attenuation;
   point_params(positionWS, light_spot_pos, light_spot_inv_range_sqr,
                light_dirWS, dist_attenuation);

   const float outer_cone = light_spot_params.x;
   const float theta = max(dot(-light_dirWS, light_spot_dir.xyz), 0.0);
   const float cone_falloff = saturate((theta - outer_cone) * light_spot_params.z);

   attenuation = dist_attenuation * cone_falloff;
}

float ggx_distribution(float NdotH, float roughness)
{
   const float roughness_sq = roughness * roughness;
   const float f = (NdotH * roughness_sq - NdotH) * NdotH + 1.0;

   return (f > 0.0) ? roughness_sq / (PI * f * f) : 0.0;
}

float ggx_occlusion(float NdotL, float NdotV, float roughness)
{
   const float roughness_sq = roughness * roughness;

   const float ggxl =
      NdotV * sqrt(NdotL * NdotL * (1.0 - roughness_sq) + roughness_sq);
   const float ggxv =
      NdotL * sqrt(NdotV * NdotV * (1.0 - roughness_sq) + roughness_sq);
   const float ggx = ggxl + ggxv;

   return (ggx > 0.0) ? 0.5 / ggx : 0.0;
}

float3 fresnel(float3 F0, float3 L, float3 H)
{
   const float LdotH = saturate(dot(L, H));
   const float3 fresnel = F0 + (1.0 - F0) * pow(1.0 - LdotH, 5);

   return fresnel * saturate(dot(F0, 333.0f));
}

float3 diffuse_term(float3 albedo)
{
   return albedo / PI;
}

float3 brdf_light(float3 diffuse, float3 F0, float roughness, float3 N, float3 L,
                  float3 V, float NdotV, float light_atten, float3 light_color)
{
   const float3 H = normalize(L + V);

   const float NdotL = saturate(dot(N, L));
   const float NdotH = saturate(dot(N, H));

   const float D = ggx_distribution(NdotH, roughness);
   const float G = ggx_occlusion(NdotL, NdotV, roughness);
   const float3 F = fresnel(F0, L, H);

   const float3 spec_contrib = F * G * D;
   const float3 diffuse_contrib = diffuse * (1.0 - F);

   return NdotL * light_atten * light_color * (spec_contrib + diffuse_contrib);
}

float3 calculate(float3 normalWS, float3 view_normalWS, float3 positionWS,
                 float3 albedo, float metallicness, float perceptual_roughness,
                 float ao, float shadow)
{
   const float roughness =
      max(perceptual_roughness * perceptual_roughness, min_roughness);
   const float dielectric = 0.04;
   const float3 F0 = lerp(dielectric, albedo, metallicness);
   albedo = lerp(albedo * (1 - dielectric), 0.0, metallicness);

   const float3 diffuse = diffuse_term(albedo);
   const float NdotV = saturate(abs(dot(normalWS, view_normalWS)));

   float3 light = 0.0;

   if (ps_light_active_directional) {
      light += brdf_light(diffuse, F0, roughness, normalWS,
                          -light_directional_0_dir.xyz, view_normalWS, NdotV,
                          1.0, light_directional_0_color.rgb);

      light += brdf_light(diffuse, F0, roughness, normalWS,
                          -light_directional_1_dir.xyz, view_normalWS, NdotV,
                          1.0, light_directional_1_color.rgb);

      light *= shadow;

      if (ps_light_active_point_0) {
         float3 light_dirWS;
         float attenuation;

         point_params(positionWS, light_point_0_pos,
                      light_point_0_inv_range_sqr, light_dirWS, attenuation);

         light += brdf_light(diffuse, F0, roughness, normalWS, light_dirWS, view_normalWS,
                             NdotV, attenuation, light_point_0_color.rgb);
      }

      if (ps_light_active_point_1) {
         float3 light_dirWS;
         float attenuation;

         point_params(positionWS, light_point_1_pos,
                      light_point_1_inv_range_sqr, light_dirWS, attenuation);

         light += brdf_light(diffuse, F0, roughness, normalWS, light_dirWS, view_normalWS,
                             NdotV, attenuation, light_point_1_color.rgb);
      }

      if (ps_light_active_point_23) {
         float3 light_dirWS;
         float attenuation;

         point_params(positionWS, light_point_2_pos,
                      light_point_2_inv_range_sqr, light_dirWS, attenuation);

         light += brdf_light(diffuse, F0, roughness, normalWS, light_dirWS, view_normalWS,
                             NdotV, attenuation, light_point_2_color.rgb);

         point_params(positionWS, light_point_3_pos,
                      light_point_3_inv_range_sqr, light_dirWS, attenuation);

         light += brdf_light(diffuse, F0, roughness, normalWS, light_dirWS, view_normalWS,
                             NdotV, attenuation, light_point_3_color.rgb);
      }
      else if (ps_light_active_spot_light) {
         float3 light_dirWS;
         float attenuation;

         spot_params(positionWS, attenuation, light_dirWS);

         light += brdf_light(diffuse, F0, roughness, normalWS, light_dirWS,
                             view_normalWS, NdotV, attenuation, light_spot_color.rgb);
      }

      light += (ambient(normalWS) * diffuse * ao);
   }
   else {
      light = 0.0;
   }

   return light;
}

}

}

#endif