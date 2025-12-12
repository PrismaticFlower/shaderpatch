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
                     float3 normalWS, float3 positionWS, float3 viewWS,
                     Point_light light, float exponent)
{
   const float3 unnormalized_light_vector = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vector);
   const float attenuation =
      attenuation_point(unnormalized_light_vector, light.inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normalWS, viewWS, light_dirWS,
             attenuation, light.color, light.stencil_shadow_factor(), exponent);
}

void calculate_spot(inout float4 in_out_diffuse, inout float4 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 view_normalWS, 
                    Spot_light light, float exponent)
{
   const float3 unnormalized_light_vectorWS = light.positionWS - positionWS;
   const float3 light_dirWS = normalize(unnormalized_light_vectorWS);

   const float theta = max(dot(-light_dirWS, light.directionWS), 0.0);
   const float cone_falloff = saturate((theta - light.cone_outer_param) * light.cone_inner_param);

   const float attenuation = attenuation_point(unnormalized_light_vectorWS, light.inv_range_sq);

   calculate(in_out_diffuse, in_out_specular, normalWS, view_normalWS,
             light_dirWS, attenuation * cone_falloff, light.color, 
             light.stencil_shadow_factor(), exponent);
}

}

}

#endif