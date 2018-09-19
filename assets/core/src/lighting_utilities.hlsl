#ifndef LIGHTING_UTILS_INCLUDED
#define LIGHTING_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "vertex_utilities.hlsl"

struct Lighting
{
   float3 color;
   float intensity;
};

namespace light
{

float3 ambient(float3 normalWS)
{
   float factor = normalWS.y * -0.5 + 0.5;

   float3 color;

   color.rgb = light_ambient_color_top.rgb * -factor + light_ambient_color_top.rgb;
   color.rgb = light_ambient_color_bottom.rgb * factor + color.rgb;

   return color;
}

float attenuation_point(float3 positionWS, float4 light_positionWS)
{
   const float inv_range_sq = light_positionWS.w;
   const float light_dst = distance(positionWS, light_positionWS.xyz);

   const float attenuation = saturate(1.0 - (light_dst * light_dst) * inv_range_sq);

   return attenuation * attenuation;
}

float attenuation_spot(float3 positionWS)
{
   const float3 light_dir = normalize(positionWS - light_spot_pos.xyz);

   const float attenuation = attenuation_point(positionWS, light_spot_pos);

   const float outer_cone = light_spot_params.x;

   const float theta = max(dot(light_dir, light_spot_dir.xyz), 0.0);
   const float cone_falloff = saturate((theta - outer_cone)  * light_spot_params.z);

   return attenuation * cone_falloff;
}

float intensity_directional(float3 normalWS, float4 direction)
{
   return max(dot(normalWS, -direction.xyz), 0.0);
}

float intensity_point(float3 normalWS, float3 positionWS, float4 light_positionWS)
{
   const float3 light_dir = normalize(positionWS - light_positionWS.xyz);

   const float intensity = max(dot(normalWS, -light_dir), 0.0);

   return attenuation_point(positionWS, light_positionWS)  * intensity;
}

float intensity_spot(float3 normalWS, float3 positionWS)
{
   const float3 light_dir = normalize(positionWS - light_spot_pos.xyz);

   const float intensity = max(dot(normalWS, -light_dir), 0.0);

   return intensity * attenuation_spot(positionWS);
}

Lighting calculate(float3 normalWS, float3 positionWS, float3 static_diffuse_lighting,
                   bool use_projected_light = false, float3 projected_light_texture_color = 0.0)
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
         intensity.y = intensity_point(normalWS, positionWS, light_point_0_pos);
         light += intensity.y * light_point_0_color;
      }

      if (ps_light_active_point_1) {
         intensity.w = intensity_point(normalWS, positionWS, light_point_1_pos);
         light += intensity.w * light_point_1_color;
      }

      if (ps_light_active_point_23) {
         intensity.w = intensity_point(normalWS, positionWS, light_point_2_pos);
         light += intensity.w * light_point_2_color;

         intensity.w = intensity_point(normalWS, positionWS, light_point_3_pos);
         light += intensity.w * light_point_3_color;
      }
      else if (ps_light_active_spot_light) {
         intensity.z = intensity_spot(normalWS, positionWS);
         light += intensity.z * light_spot_color;
      }

      if (use_projected_light) {
         const float proj_light_intensity = dot(light_proj_selector, intensity);

         light.rgb -= (light_proj_color.rgb * proj_light_intensity);
         light.rgb += (projected_light_texture_color * light_proj_color.rgb * proj_light_intensity);
      }

      float scale = max(max(light.r, light.g), light.b);
      scale = max(scale, 1.0);
      light.rgb = lerp(light.rgb / scale, light.rgb, tonemap_state);
      light.rgb *= hdr_info.z;

      lighting.color = light.rgb;
      lighting.intensity = light.a;
   }
   else {
      lighting.color = hdr_info.zzz;
      lighting.intensity = 0.0;
   }

   return lighting;
}

namespace pbr
{

static const float PI = 3.14159265359f;

float normal_distribution(float roughness, float NdotH)
{
   const float a = roughness * roughness;

   const float d = (NdotH * NdotH) * ((a * a) - 1.0) + 1.0;

   return (a * a) / (PI * (d * d));
}

float geometric_occlusion(float roughness, float NdotL, float NdotV)
{
   const float k = (roughness + 1) * (roughness + 1) / 8;

   const float g1 = NdotL / (NdotL * (1 - k) + k);
   const float g2 = NdotV / (NdotV * (1 - k) + k);

   return g1 * g2;
}

float3 fresnel_schlick(float3 F0, float3 V, float3 H)
{
   const float VdotH = saturate(dot(V, H));

   return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5);
}

float3 diffuse_term(float3 albedo)
{
   return albedo / PI;
}

float3 brdf_light(float3 diffuse, float3 F0, float roughness, float3 N, float3 L, float3 V,
                  float NdotV, float light_atten, float3 light_color)
{
   const float3 H = normalize(L + V);

   const float NdotL = saturate(dot(N, L));
   const float NdotH = saturate(dot(N, H));

   const float D = normal_distribution(roughness, NdotH);
   const float G = geometric_occlusion(roughness, NdotL, NdotV);
   const float3 F = fresnel_schlick(F0, V, H);

   const float3 spec_contrib = (D * G * F) / (4.0 * NdotL * NdotV);
   const float3 diffuse_contrib = (1.0 - F) * diffuse;

   return NdotL * light_atten * light_color * (spec_contrib + diffuse_contrib);
}

float3 calculate(float3 N, float3 V, float3 world_position, float3 albedo,
                  float metallicness, float roughness, float ao, float shadow)
{
   const float dielectric = 0.04;
   const float3 F0 = lerp(dielectric, albedo, metallicness);
   albedo = lerp(albedo * (1 - dielectric), 0.0, metallicness);

   const float3 diffuse = diffuse_term(albedo);
   const float NdotV = saturate(dot(N, V));

   float3 light = 0.0;

   if (ps_light_active_directional) {
      light += brdf_light(diffuse, F0, roughness, N, -light_directional_0_dir.xyz,
                          V, NdotV, 1.0, light_directional_0_color.rgb);

      light += brdf_light(diffuse, F0, roughness, N, -light_directional_1_dir.xyz,
                          V, NdotV, 1.0, light_directional_1_color.rgb);

      light *= shadow;

      if (ps_light_active_point_0) {
         const float3 light_dir = normalize(light_point_0_pos.xyz - world_position);
         const float attenuation = attenuation_point(world_position, light_point_0_pos);

         light += brdf_light(diffuse, F0, roughness, N, light_dir,
                             V, NdotV, attenuation, light_point_0_color.rgb);
      }

      if (ps_light_active_point_1) {
         const float3 light_dir = normalize(light_point_1_pos.xyz - world_position);
         const float attenuation = attenuation_point(world_position, light_point_1_pos);

         light += brdf_light(diffuse, F0, roughness, N, light_dir,
                             V, NdotV, attenuation, light_point_1_color.rgb);
      }

      if (ps_light_active_point_23) {
         float3 light_dir = normalize(light_point_2_pos.xyz - world_position);
         float attenuation = attenuation_point(world_position, light_point_2_pos);

         light += brdf_light(diffuse, F0, roughness, N, light_dir,
                             V, NdotV, attenuation, light_point_2_color.rgb);

         light_dir = normalize(light_point_3_pos.xyz - world_position);
         attenuation = attenuation_point(world_position, light_point_3_pos);

         light += brdf_light(diffuse, F0, roughness, N, light_dir,
                             V, NdotV, attenuation, light_point_3_color.rgb);
      }
      else if (ps_light_active_spot_light) {
         const float3 light_dir = normalize(light_spot_pos.xyz - world_position);
         const float attenuation = attenuation_spot(world_position);

         light += brdf_light(diffuse, F0, roughness, N, light_dir,
                             V, NdotV, attenuation, light_spot_color.rgb);
      }

      light += (ambient(N) * diffuse * ao);
   }
   else {
      light = 0.0;
   }

   return light;
}

}

}

#endif