#ifndef LIGHTING_UTILS_INCLUDED
#define LIGHTING_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "vertex_utilities.hlsl"

struct Lighting
{
   float4 diffuse;
   float4 static_diffuse;
};

struct Pixel_lighting
{
   float3 color;
   float intensity;
};

namespace light
{

float3 ambient(float3 world_normal)
{
   float factor = world_normal.y * -0.5 + 0.5;

   float3 color;

   color.rgb = light_ambient_color_top.rgb * -factor + light_ambient_color_top.rgb;
   color.rgb = light_ambient_color_bottom.rgb * factor + color.rgb;

   return color;
}

float attenuation_point(float3 world_position, float4 light_position)
{
   const float inv_range_sq = light_position.w;
   const float light_dst = distance(world_position, light_position.xyz);

   return max(1.0 - (light_dst * light_dst) * inv_range_sq, 0.0);
}

float attenuation_spot(float3 world_position)
{
   const float3 light_dir = normalize(world_position - light_spot_pos.xyz);

   const float inv_range_sq = light_spot_pos.w;
   const float light_dst = distance(world_position, light_spot_pos.xyz);

   const float attenuation = max(1.0 - (light_dst * light_dst) * inv_range_sq, 0.0);

   const float outer_cone = light_spot_params.x;

   const float theta = max(dot(light_dir, light_spot_dir.xyz), 0.0);
   const float cone_falloff = saturate((theta - outer_cone)  * light_spot_params.z);

   return attenuation * cone_falloff;
}

float intensity_directional(float3 world_normal, float4 direction)
{
   return max(dot(world_normal, -direction.xyz), 0.0);
}

float intensity_point(float3 world_normal, float3 world_position, float4 light_position)
{
   const float3 light_dir = normalize(world_position - light_position.xyz);

   const float intensity = max(dot(world_normal.xyz, -light_dir), 0.0);

   return attenuation_point(world_position, light_position)  * intensity;
}

float intensity_spot(float3 world_normal, float3 world_position)
{
   const float3 light_dir = normalize(world_position - light_spot_pos.xyz);

   const float intensity = max(dot(world_normal, -light_dir), 0.0);

   return intensity * attenuation_spot(world_position);
}

Lighting calculate(float3 normals, float3 world_position,
                   float4 static_diffuse_lighting)
{
   float3 world_normal = normals_to_world(normals);

   Lighting lighting;

   lighting.diffuse = 0.0;
   lighting.diffuse.rgb = ambient(world_normal) + static_diffuse_lighting.rgb;

#ifdef LIGHTING_DIRECTIONAL
   float4 intensity = float4(lighting.diffuse.rgb, 1.0);

   intensity.x = intensity_directional(world_normal, light_directional_0_dir);
   lighting.diffuse += intensity.x * light_directional_0_color;

   intensity.w = intensity_directional(world_normal, light_directional_1_dir);
   lighting.diffuse += intensity.w * light_directional_1_color;

#ifdef LIGHTING_POINT_0
   intensity.y = intensity_point(world_normal, world_position, light_point_0_pos);
   lighting.diffuse += intensity.y * light_point_0_color;
#endif

#ifdef LIGHTING_POINT_1
   intensity.w = intensity_point(world_normal, world_position, light_point_1_pos);
   lighting.diffuse += intensity.w * light_point_1_color;
#endif

#ifdef LIGHTING_POINT_23
   intensity.w = intensity_point(world_normal, world_position, light_point_2_pos);
   lighting.diffuse += intensity.w * light_point_2_color;

   intensity.w = intensity_point(world_normal, world_position, light_point_3_pos);
   lighting.diffuse += intensity.w * light_point_3_color;
#elif defined(LIGHTING_SPOT_0)
   intensity.z = intensity_spot(world_normal, world_position);
   lighting.diffuse += intensity.z * light_spot_color;
#endif

   lighting.static_diffuse = static_diffuse_lighting;
   lighting.static_diffuse.w = dot(light_proj_selector, intensity);
   lighting.diffuse.rgb += -light_proj_color.rgb * lighting.static_diffuse.w;

   float scale = max(lighting.diffuse.r, lighting.diffuse.g);
   scale = max(scale, lighting.diffuse.b);
   scale = max(scale, 1.0);
   scale = rcp(scale);
   lighting.diffuse.rgb *= scale;
   lighting.diffuse.rgb *= hdr_info.z;
#else // LIGHTING_DIRECTIONAL

   lighting.diffuse = float4(1.0.xxx, 0.0);
   lighting.static_diffuse = 0.0;
#endif

   return lighting;
}

Lighting vertex_precalculate(float3 world_normal, float3 world_position,
                             float4 static_diffuse_lighting)
{
   Lighting lighting;

   lighting.diffuse = 0.0;
   lighting.diffuse.rgb = ambient(world_normal) + static_diffuse_lighting.rgb;

#ifdef LIGHTING_DIRECTIONAL
   float4 intensity = float4(lighting.diffuse.rgb, 1.0);

   intensity.x = intensity_directional(world_normal, light_directional_0_dir);
   intensity.w = intensity_directional(world_normal, light_directional_1_dir);
#ifdef LIGHTING_POINT_0
   intensity.y = intensity_point(world_normal, world_position, light_point_0_pos);
#endif

#ifdef LIGHTING_POINT_1
   intensity.w = intensity_point(world_normal, world_position, light_point_1_pos);
#endif

#ifdef LIGHTING_POINT_23
   intensity.w = intensity_point(world_normal, world_position, light_point_3_pos);
#elif defined(LIGHTING_SPOT_0)
   intensity.z = intensity_spot(world_normal, world_position);
#endif
   lighting.static_diffuse = static_diffuse_lighting;
   lighting.static_diffuse.w = dot(light_proj_selector, intensity);
   lighting.diffuse.rgb += -light_proj_color.rgb * lighting.static_diffuse.w;

#else // LIGHTING_DIRECTIONAL

   lighting.diffuse = float4(1.0.xxx, 0.0);
   lighting.static_diffuse = 0.0;
#endif

   return lighting;
}

Pixel_lighting pixel_calculate(float3 world_normal, float3 world_position, 
                               float3 precalculated)
{
   float4 light = float4(precalculated, 0.0);

   float4 intensity = float4(light.rgb, 1.0);

   [branch] if (directional_lights) {
      intensity.x = intensity_directional(world_normal, light_directional_0_dir);
      light += intensity.x * light_directional_0_color;

      intensity.w = intensity_directional(world_normal, light_directional_1_dir);
      light += intensity.w * light_directional_1_color;


      [branch] if (point_light_0) {
         intensity.y = intensity_point(world_normal, world_position, light_point_0_pos);
         light += intensity.y * light_point_0_color;
      }

      [branch] if (point_light_1) {
         intensity.w = intensity_point(world_normal, world_position, light_point_1_pos);
         light += intensity.w * light_point_1_color;
      }

      [branch] if (point_light_23) {
         intensity.w = intensity_point(world_normal, world_position, light_point_2_pos);
         light += intensity.w * light_point_2_color;


         intensity.w = intensity_point(world_normal, world_position, light_point_3_pos);
         light += intensity.w * light_point_3_color;
      }
      else if (spot_light) {
         intensity.z = intensity_spot(world_normal, world_position);
         light += intensity.z * light_spot_color;
      }

      float scale = max(light.r, light.g);
      scale = max(scale, light.b);
      scale = max(scale, 1.0);
      scale = rcp(scale);
      light.rgb *= scale;
      light.rgb *= hdr_info.z;
   }
   else {
      light = 1.0;
   }

   Pixel_lighting result;

   result.color = light.rgb;
   result.intensity = light.a;

   return result;
}

namespace pbr
{

static const float PI = 3.14159265359f;

float normal_distribution(float roughness, float3 N, float3 H)
{
   const float a = roughness * roughness;
   const float NdotH = saturate(dot(N, H));

   const float d = (NdotH * NdotH) * ((a * a) - 1.0) + 1.0;

   return (a * a) / (PI * (d * d));
}

float geometric_occlusion(float roughness, float3 N, float3 L, float3 V)
{
   const float k = (roughness + 1) * (roughness + 1) / 8;

   const float NdotL = saturate(dot(N, L));
   const float NdotV = saturate(dot(N, V));

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

float3 brdf_light(float3 albedo, float3 F0, float roughness, float3 N, float3 L, float3 V,
                  float light_atten, float3 light_color)
{
   const float3 H = normalize(L + V);

   const float D = normal_distribution(roughness, N, H);
   const float G = geometric_occlusion(roughness, N, L, V);
   const float3 F = fresnel_schlick(F0, V, H);

   const float NdotL = saturate(dot(N, L));
   const float NdotV = saturate(dot(N, V));

   const float3 spec = (D * G * F) / (4.0 * NdotL * NdotV);
   const float3 diffuse = (1.0 - F) * diffuse_term(albedo);

   return NdotL * light_atten * light_color * (spec + diffuse);
}

float3 calculate(float3 world_normal, float3 world_position, float3 albedo,
                 float roughness, float metallic, float ao, float shadow, float3 V)
{
   const float dielectric = 0.04;
   const float3 F0 = lerp(dielectric, albedo, metallic);
   albedo = lerp(albedo * (1 - dielectric), 0.0, metallic);

   float3 light = 0.0;

   [branch] if (directional_lights) {
      light += brdf_light(albedo, F0, roughness, world_normal, -light_directional_0_dir.xyz,
                          V, 1.0, light_directional_0_color.rgb);

      light += brdf_light(albedo, F0, roughness, world_normal, -light_directional_1_dir.xyz,
                          V, 1.0, light_directional_1_color.rgb);

      [branch] if (point_light_0) {
         const float3 light_dir = normalize(light_point_0_pos.xyz - world_position);
         const float attenuation = attenuation_point(world_position, light_point_0_pos);

         light += brdf_light(albedo, F0, roughness, world_normal, light_dir,
                             V, attenuation, light_point_0_color.rgb);
      }

      [branch] if (point_light_1) {
         const float3 light_dir = normalize(light_point_1_pos.xyz - world_position);
         const float attenuation = attenuation_point(world_position, light_point_1_pos);

         light += brdf_light(albedo, F0, roughness, world_normal, light_dir,
                             V, attenuation, light_point_1_color.rgb);
      }

      [branch] if (point_light_23) {
         float3 light_dir = normalize(light_point_2_pos.xyz - world_position);
         float attenuation = attenuation_point(world_position, light_point_2_pos);

         light += brdf_light(albedo, F0, roughness, world_normal, light_dir,
                             V, attenuation, light_point_2_color.rgb);

         light_dir = normalize(light_point_3_pos.xyz - world_position);
         attenuation = attenuation_point(world_position, light_point_3_pos);

         light += brdf_light(albedo, F0, roughness, world_normal, light_dir,
                             V, attenuation, light_point_3_color.rgb);
      }
      else if (spot_light) {
         const float3 light_dir = normalize(light_spot_pos.xyz - world_position);
         const float attenuation = attenuation_spot(world_position);

         light += brdf_light(albedo, F0, roughness, world_normal, light_dir,
                             V, attenuation, light_spot_color.rgb);
      }
   }
   else {
      light = 1.0;
   }

   light *= shadow;
   light += (ambient(world_normal) * ao);

   return light;
}

}

}

#endif