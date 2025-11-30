#ifndef LIGHTING_BLINNPHONG_EXT_INCLUDED
#define LIGHTING_BLINNPHONG_EXT_INCLUDED

#include "lighting_utilities.hlsl"

namespace light {

namespace blinnphong_normalized {

void calculate(inout float4 in_out_diffuse, inout float4 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float4 light_color, float exponent,
               out float out_diffuse_intensity, out float out_specular_intensity)
{
   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float LdotH = saturate(dot(L, H));

   // Christian Schüler's Minimalist Cook-Torrance - http://www.thetenthplanet.de/archives/255
   const float D = (exponent + 1.0) * pow(NdotH, exponent) * 0.5;
   const float FV = 0.25 * pow(LdotH, 3.0);
   const float specular = D * FV * attenuation;
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


void calculate(inout float3 in_out_diffuse, inout float3 in_out_specular,
               float3 N, float3 V, float3 L, float attenuation,
               float3 light_color, float exponent)
{
   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float LdotH = saturate(dot(L, H));

   // Christian Schüler's Minimalist Cook-Torrance - http://www.thetenthplanet.de/archives/255
   const float D = (exponent + 1.0) * pow(NdotH, exponent) * 0.5;
   const float FV = 0.25 * pow(LdotH, 3.0);
   const float specular = D * FV * attenuation;
   const float diffuse = saturate(dot(N, L)) * attenuation;

   in_out_diffuse += (diffuse * light_color);
   in_out_specular += (specular * light_color);
}

void calculate_point(inout float3 in_out_diffuse, inout float3 in_out_specular,
                     float3 normal, float3 position, float3 view_normal,
                     float3 light_position, float inv_range_sqr, float3 light_color,
                     float exponent)
{
   const float3 unnormalized_light_vector = light_position - position;
   const float3 light_dir = normalize(unnormalized_light_vector);
   const float attenuation =
      attenuation_point(unnormalized_light_vector, inv_range_sqr);

   calculate(in_out_diffuse, in_out_specular, normal, view_normal, light_dir,
             attenuation, light_color, exponent);
}

void calculate_spot(inout float3 in_out_diffuse, inout float3 in_out_specular,
                    float3 normalWS, float3 positionWS, float3 view_normalWS, float3 light_positionWS, 
                    float inv_range_sqr, float3 light_directionWS, float outer_spot_param, float inner_spot_param, 
                    float3 light_color, float exponent)
{
   const float3 unnormalized_light_vectorWS = light_positionWS - positionWS;

   const float3 light_vectorWS = normalize(unnormalized_light_vectorWS);

   const float theta = max(dot(-light_vectorWS, light_directionWS), 0.0);
   const float cone_falloff = saturate((theta - outer_spot_param) * inner_spot_param);

   const float attenuation = 
      attenuation_point(unnormalized_light_vectorWS, inv_range_sqr) * cone_falloff;


   calculate(in_out_diffuse, in_out_specular, normalWS, view_normalWS,
             light_vectorWS, attenuation, light_color, exponent);
}

}

}

#endif