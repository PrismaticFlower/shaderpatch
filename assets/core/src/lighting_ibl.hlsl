#ifndef LIGHTING_IBL_INCLUDED
#define LIGHTING_IBL_INCLUDED

#include "lighting_brdf.hlsl"
#include "lighting_occlusion.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float2> ibl_dfg : PS_MATERIAL_REGISTER(5);
TextureCube<float3> ibl_specular : PS_MATERIAL_REGISTER(6);
TextureCube<float3> ibl_diffuse : PS_MATERIAL_REGISTER(7);

namespace pbr {

float3 evaluate_ibl(brdf_params params, float ao, float perceptual_roughness)
{
   float3 reflected = reflect(-params.V, params.N);

   float NdotV = saturate(dot(params.N, params.V));
   perceptual_roughness = saturate(perceptual_roughness);

   float2 dfg = ibl_dfg.SampleLevel(linear_clamp_sampler,
                                    float2(NdotV, perceptual_roughness), 0);

   float specular_lod = 8.0 * perceptual_roughness;
   float3 specular_sample =
      ibl_specular.SampleLevel(linear_wrap_sampler, reflected, specular_lod);

   float3 specular = specular_sample * (params.f0 * dfg.x + dfg.y);

   float3 diffuse = ibl_diffuse.Sample(linear_wrap_sampler, reflected) *
                    params.diffuse_color * diffuse_lambert();

   float3 multi_ao = gtao_multi_bounce(ao, params.diffuse_color);
   float3 multi_so =
      gtao_multi_bounce(specular_occlusion(NdotV, ao, params.roughness), params.f0);

   return (diffuse * multi_ao) + (specular * multi_so);
}

}

#endif