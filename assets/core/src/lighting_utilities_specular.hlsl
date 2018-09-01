#ifndef NORMAL_EXT_LIGHTING_UTILS_INCLUDED
#define NORMAL_EXT_LIGHTING_UTILS_INCLUDED

#include "lighting_utilities.hlsl"

namespace light {
namespace specular {

float3 calculate(out float out_intensity, float3 world_normal, float3 view_normal,
                 float3 light_direction, float3 light_color, float3 diffuse_color, 
                 float3 specular_color, float exponent)
{
   const float3 H = normalize(light_direction + view_normal);
   const float NdotH = max(dot(world_normal, H), 0.0);
   float specular = pow(NdotH, exponent);
   float diffuse = max(dot(world_normal, light_direction), 0.0);

   out_intensity = diffuse + specular;

   return (diffuse * diffuse_color * light_color) + (specular * specular_color * light_color);
}

float3 calculate_point(out float out_intensity, float3 world_normal, float3 world_position,
                       float3 view_normal, float4 light_position, float3 light_color,
                       float3 diffuse_color, float3 specular_color, float exponent)
{
   const float3 light_dir = -normalize(world_position - light_position.xyz);
   const float attenuation = attenuation_point(world_position, light_position);
   
   float intensity;
   const float3 color = calculate(intensity, world_normal, view_normal, 
                                  light_dir, light_color, diffuse_color, 
                                  specular_color, exponent);
   out_intensity = intensity * attenuation;

   return color * attenuation;
}

float3 calculate_spot(out float out_intensity, float3 world_normal, float3 world_position,
                      float3 view_normal, float3 diffuse_color, float3 specular_color, 
                      float exponent)
{
   const float3 light_dir = -normalize(world_position - light_spot_pos.xyz);
   const float attenuation = attenuation_spot(world_position);

   float intensity;
   const float3 color = calculate(intensity, world_normal, view_normal, 
                                  light_dir, light_spot_color.rgb, diffuse_color,
                                  specular_color, exponent);
   out_intensity = intensity * attenuation;

   return color * attenuation;
}

}
}

#endif