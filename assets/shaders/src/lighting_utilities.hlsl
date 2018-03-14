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

float intensity_directional(float3 world_normal, float4 direction)
{
   float intensity = dot(world_normal.xyz, -direction.xyz);

   return max(intensity, 0.0);
}

float intensity_point(float3 world_normal, float3 world_position, float4 light_position)
{
   float3 light_dir = world_position - light_position.xyz;

   const float dir_dot = dot(light_dir, light_dir);

   light_dir *= rsqrt(dir_dot);

   float3 intensity;

   const float inv_range_sq = light_position.w;

   intensity.x = 1.0;
   intensity.z = -dir_dot * inv_range_sq + intensity.x;
   intensity.y = dot(world_normal.xyz, -light_dir);
   intensity = max(intensity, 0.0);

   return intensity.y * intensity.z;
}

float intensity_spot(float3 world_normal, float3 world_position)
{
   const float inv_range_sq = light_spot_pos.w;
   const float bidirectional = light_spot_dir.w;

   // find light direction
   float3 light_dir = world_position + -light_spot_pos.xyz;

   const float dir_dot = dot(light_dir, light_dir);
   const float dir_rsqr = rsqrt(dir_dot);

   light_dir *= dir_rsqr;

   // calculate angular attenuation
   float4 attenuation;

   attenuation = dot(light_dir, light_spot_dir.xyz);
   attenuation.x = (dir_rsqr < 0.0) ? 1.0f : 0.0f;
   attenuation.y = 1.0;
   attenuation.x = bidirectional * -attenuation.x + attenuation.y;
   attenuation.w = attenuation.w * attenuation.x;

   // compute distance attenuation
   attenuation.z = -dir_dot * inv_range_sq + attenuation.y;
   attenuation = max(attenuation, 0.0);

   // set if inside the inner/outer cone
   attenuation.y = (attenuation.w >= light_spot_params.x) ? 1.0f : 0.0f;
   attenuation.x = (attenuation.w <  0.0) ? 1.0f : 0.0f;
   attenuation.z *= attenuation.y;

   // compute the falloff if inbetween the inner and outer cone
   attenuation.y = attenuation.w + -light_spot_params.x;
   attenuation.y *= light_spot_params.z;
   attenuation.w *= light_spot_params.w * attenuation.x;
   attenuation.x = dot(world_normal, -light_dir);

   float4 coefficient = lit(attenuation.x, attenuation.y, attenuation.w);

   // calculate spot attenuated intensity
   attenuation = max(attenuation, 0.0);

   return (attenuation.z * coefficient.z) * attenuation.x;
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

}

#endif