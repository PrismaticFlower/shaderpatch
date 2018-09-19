#ifndef NORMAL_EXT_LIGHTING_UTILS_INCLUDED
#define NORMAL_EXT_LIGHTING_UTILS_INCLUDED

#include "lighting_utilities.hlsl"

namespace light {
namespace specular {

void calculate(out float out_diffuse_intensity, out float out_specular_intensity, 
               inout float3 in_out_diffuse, inout float3 in_out_specular, 
               float3 N, float3 V, float3 L, float attenuation, float3 light_color, 
               float exponent)
{
   const float3 H = normalize(L + V);
   const float NdotH = max(dot(N, H), 0.0);
   const float specular = pow(NdotH, exponent) * attenuation;
   const float diffuse = max(dot(N, L), 0.0) * attenuation;

   out_diffuse_intensity = diffuse;
   out_specular_intensity = specular;

   in_out_diffuse += (diffuse * light_color);
   in_out_specular += (specular * light_color);
}

void calculate_point(out float out_diffuse_intensity, out float out_specular_intensity,
                     inout float3 in_out_diffuse, inout float3 in_out_specular, float3 normal,
                     float3 position, float3 view_normal, float4 light_position, 
                     float3 light_color, float exponent)
{
   const float3 light_dir = -normalize(position - light_position.xyz);
   const float attenuation = attenuation_point(position, light_position);
   
   calculate(out_diffuse_intensity, out_specular_intensity, in_out_diffuse, in_out_specular,
             normal, view_normal, light_dir, attenuation, light_color.rgb, exponent);
}

void calculate_spot(out float out_diffuse_intensity, out float out_specular_intensity,
                    inout float3 in_out_diffuse, inout float3 in_out_specular, 
                    float3 normalWS, float3 positionWS, float3 view_normalWS, float exponent)
{
   const float3 light_dirWS = -normalize(positionWS - light_spot_pos.xyz);
   const float attenuation = attenuation_spot(positionWS);

   calculate(out_diffuse_intensity, out_specular_intensity, in_out_diffuse, in_out_specular,
             normalWS, view_normalWS, light_dirWS, attenuation, light_spot_color.rgb, 
             exponent);
}

}
}

#endif