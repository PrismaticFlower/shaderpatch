#ifndef LIGHTING_BRDF_INCLUDED
#define LIGHTING_BRDF_INCLUDED

namespace pbr {

static const float PI = 3.14159265359;

struct brdf_params {
   float3 N;
   float3 V;

   float roughness;
   float roughness_sq;
   float3 f0;
   float3 diffuse_color;

   float NdotV;
};

float D_ggx(float NdotH, float roughness)
{
   const float one_minus_NdotH_sq = 1.0 - NdotH * NdotH;
   const float a = NdotH * roughness;
   const float k = roughness / (one_minus_NdotH_sq + a * a);

   return k * k * (1.0 / PI);
}

float V_smith_ggx_correlated(float NdotL, float NdotV, float a2)
{
   const float lambda_v = NdotL * sqrt((NdotV - a2 * NdotV) * NdotV + a2);
   const float lambda_l = NdotV * sqrt((NdotL - a2 * NdotL) * NdotL + a2);

   return 0.5 / (lambda_v + lambda_l);
}

float3 F_schlick(float3 f0, float v)
{
   const float f = pow(1.0 - v, 5);

   return f + f0 * (1.0 - f);
}

float3 F_schlick(float3 f0, float3 L, float3 H)
{
   return F_schlick(f0, saturate(dot(L, H)));
}

float diffuse_lambert()
{
   return 1.0 / PI;
}

}

#endif