#ifndef LIGHTING_BLINNPHONG_EXT_INCLUDED
#define LIGHTING_BLINNPHONG_EXT_INCLUDED

#include "lighting_utilities.hlsl"

namespace light {

namespace blinnphong_normalized {

void calculate(inout float4 in_out_diffuse, inout float4 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float3 light_color, float shadow_factor, float exponent)
{
   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float LdotH = saturate(dot(L, H));

   // Christian Schüler's Minimalist Cook-Torrance - http://www.thetenthplanet.de/archives/255
   const float D = (exponent + 1.0) * pow(NdotH, exponent) * 0.5;
   const float FV = 0.25 * pow(LdotH, 3.0);
   const float specular = D * FV * attenuation;
   const float diffuse = saturate(dot(N, L)) * attenuation;

   in_out_diffuse += (diffuse * float4(light_color, shadow_factor));
   in_out_specular += (specular * float4(light_color, shadow_factor));
}

void calculate_point(inout float4 in_out_diffuse, inout float4 in_out_specular,
                     float3 normal, float3 position, float3 view_normal,
                     float3 light_position, float inv_range_sq, float3 light_color,
                     float shadow_factor, float exponent)
{
   const float3 unnormalized_light_vector = light_position - position;
   const float3 light_dir = normalize(unnormalized_light_vector);
   const float attenuation =
      attenuation_point(unnormalized_light_vector, inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normal, view_normal, light_dir,
             attenuation, light_color, shadow_factor, exponent);
}

void calculate_spot(inout float4 in_out_diffuse, inout float4 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 view_normalWS,
                    float3 light_positionWS, float inv_range_sq, float3 cone_directionWS, 
                    float cone_outer_param, float cone_inner_param,
                    float3 light_color, float shadow_factor, float exponent)
{
   const float3 unnormalized_light_vectorWS = light_positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);

   const float outer_cone = light_spot_params.x;
   
   const float theta = max(dot(-light_dirWS, cone_directionWS), 0.0);
   const float cone_falloff = saturate((theta - cone_outer_param) * cone_inner_param);

   const float attenuation = attenuation_point(unnormalized_light_vectorWS, inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normalWS, view_normalWS,
             light_dirWS, attenuation * cone_falloff, light_color, 
             shadow_factor, exponent);
}

}

}

#endif