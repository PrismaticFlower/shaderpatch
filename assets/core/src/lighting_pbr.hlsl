#ifndef LIGHTING_PBR_INCLUDED
#define LIGHTING_PBR_INCLUDED

#include "constants_list.hlsl"
#include "lighting_brdf.hlsl"
#include "lighting_ibl.hlsl"
#include "lighting_utilities.hlsl"

#pragma warning(disable : 3078) // **sigh**...: loop control variable conflicts with a previous declaration in the outer scope

namespace pbr {

struct surface_info {
   float3 normalWS;
   float3 viewWS;
   float3 positionWS;

   float3 base_color;
   float metallicness;
   float perceptual_roughness;

   float ao;
   float sun_shadow;

   bool use_ibl;
};

static const float min_roughness = 0.045;
static const float dielectric_reflectance = 0.04;

float attenuation_point_smooth(float distance_sqr, float inv_range_sqr)
{
   const float factor = distance_sqr * inv_range_sqr;
   const float smooth_factor = saturate(1.0 - factor * factor);

   return smooth_factor * smooth_factor;
}

float attenuation_point(float3 unnormalized_light_vector, float inv_range_sqr)
{
   const float dist_sqr = dot(unnormalized_light_vector, unnormalized_light_vector);
   const float attenuation = 1.0 / max(dist_sqr, 0.01 * 0.01);

   return attenuation * attenuation_point_smooth(dist_sqr, inv_range_sqr);
}

void point_params(float3 position, float3 light_position, float inv_range_sqr,
                  out float3 light_dir, out float light_attenuation)
{
   const float3 unnormalized_light_vector = light_position - position;

   light_dir = normalize(unnormalized_light_vector);
   light_attenuation = attenuation_point(unnormalized_light_vector, inv_range_sqr);
}

void spot_params(float3 positionWS, out float attenuation, out float3 light_dirWS)
{
   float dist_attenuation;
   point_params(positionWS, light_spot_pos, light_spot_inv_range_sqr,
                light_dirWS, dist_attenuation);

   const float outer_cone = light_spot_params.x;
   const float theta = max(dot(-light_dirWS, light_spot_dir.xyz), 0.0);
   const float cone_falloff = saturate((theta - outer_cone) * light_spot_params.z);

   attenuation = dist_attenuation * cone_falloff;
}

float3 brdf_light(brdf_params params, float3 L, float light_atten, float3 light_color)
{
   const float3 H = normalize(params.V + L);

   const float NdotL = saturate(dot(params.N, L));
   const float NdotH = saturate(dot(params.N, H));
   const float LdotH = saturate(dot(L, H));

   const float D = D_ggx(NdotH, params.roughness);
   const float V = V_smith_ggx_correlated(NdotL, params.NdotV, params.roughness_sq);
   const float3 F = F_schlick(params.f0, LdotH);

   const float3 spec_contrib = D * V * F;
   const float3 diffuse_contrib =
      params.diffuse_color * diffuse_lambert() * (1.0 - F);

   return NdotL * light_atten * light_color * (spec_contrib + diffuse_contrib);
}

brdf_params get_params(surface_info surface)
{
   brdf_params params;

   params.N = surface.normalWS;
   params.V = surface.viewWS;

   params.roughness =
      max(surface.perceptual_roughness * surface.perceptual_roughness, min_roughness);
   params.roughness_sq = params.roughness * params.roughness;

   params.f0 = lerp(dielectric_reflectance, surface.base_color, surface.metallicness);

   params.diffuse_color = lerp(surface.base_color * (1 - dielectric_reflectance),
                               0.0, surface.metallicness);

   params.NdotV = abs(dot(params.N, params.V)) + 1e-5;

   return params;
}

float3 calculate(surface_info surface)
{
   brdf_params params = get_params(surface);

   float3 light = 0.0;

   [branch] if (light_active)
   {

      [loop] for (uint i = 0; i < 2; ++i)
      {
         light += brdf_light(params, -light_directional_dir(i), 1.0,
                             light_directional_color(i).rgb);
      }

      light *= surface.sun_shadow;

      [loop] for (uint i = 0; i < light_active_point_count; ++i)
      {
         float3 light_dirWS;
         float attenuation;

         point_params(surface.positionWS, light_point_pos(i),
                      light_point_inv_range_sqr(i), light_dirWS, attenuation);

         light +=
            brdf_light(params, light_dirWS, attenuation, light_point_color(i).rgb);
      }

      [branch] if (light_active_spot)
      {
         float3 light_dirWS;
         float attenuation;

         spot_params(surface.positionWS, attenuation, light_dirWS);

         light += brdf_light(params, light_dirWS, attenuation, light_spot_color.rgb);
      }

      if (surface.use_ibl) {
         light += evaluate_ibl(params, surface.ao, surface.perceptual_roughness);
      }
      else {
         light += (light::ambient(surface.normalWS) * params.diffuse_color *
                   diffuse_lambert() * surface.ao);
      }
   }
   else
   {
      light = 0.0;
   }

   return light;
}

}

#endif