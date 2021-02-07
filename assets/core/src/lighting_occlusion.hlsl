#ifndef LIGHTING_OCCLUSION_INCLUDED
#define LIGHTING_OCCLUSION_INCLUDED

namespace pbr {

float specular_occlusion(float NdotV, float ao, float roughness)
{
   return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

float3 gtao_multi_bounce(float visibility, float3 albedo)
{
   float3 a = 2.0404 * albedo - 0.3324;
   float3 b = -4.7951 * albedo + 0.6417;
   float3 c = 2.7552 * albedo + 0.6903;

   return max(visibility, ((visibility * a + b) * visibility + c) * visibility);
}

}

#endif