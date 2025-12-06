#ifndef LIGHTING_PBR_INCLUDED
#define LIGHTING_PBR_INCLUDED

#include "constants_list.hlsl"
#include "lighting_brdf.hlsl"
#include "lighting_ibl.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

#pragma warning(disable : 3078) // **sigh**...: loop control variable conflicts with a previous declaration in the outer scope

namespace pbr {

struct surface_info {
   float3 normalWS;
   float3 viewWS;
   float3 positionWS;
   float4 positionSS;

   float3 base_color;
   float metallicness;
   float perceptual_roughness;

   float ao;
   float sun_shadow;

   bool use_ibl;
};

static const float min_roughness = 0.045;
static const float dielectric_reflectance = 0.04;
static const bool  sp_use_projected_texture = SP_USE_PROJECTED_TEXTURE;

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

void spot_params(float3 positionWS, Spot_light light, out float attenuation, out float3 light_dirWS)
{
   point_params(positionWS, light.positionWS, light.inv_range_sq,
                light_dirWS, attenuation);

   const float theta = max(dot(-light_dirWS, light.directionWS), 0.0);
   const float cone_falloff = saturate((theta - light.cone_outer_param) * light.cone_inner_param);

   attenuation *= cone_falloff;
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

float3 calculate(surface_info surface, Texture2D<float3> projected_light_texture)
{
   brdf_params params = get_params(surface);

   const float3 positionWS = surface.positionWS;
   const float3 normalWS = surface.normalWS;

   float3 light = 0.0;

   [branch] 
   if (light_active) {
      Lights_context context = acquire_lights_context(positionWS, surface.positionSS);

      const float3 projected_light_texture_color = 
         sp_use_projected_texture ? sample_projected_light(projected_light_texture, 
                                                           mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                  : 0.0;

      while (!context.directional_lights_end()) {
         Directional_light directional_light = context.next_directional_light();

         float3 light_color = directional_light.color;

         if (directional_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float shadowing = 1.0;

         if (directional_light.use_sun_shadow_map()) {
            shadowing = sample_sun_shadow_map(positionWS);
         }

         light += brdf_light(params, -directional_light.directionWS, shadowing, light_color);
      }

      light *= surface.sun_shadow;

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         const float intensity = light::intensity_point(normalWS, positionWS, point_light);
         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float3 light_dirWS;
         float attenuation;

         point_params(positionWS, point_light.positionWS,
                      point_light.inv_range_sq, light_dirWS, attenuation);

         light += brdf_light(params, light_dirWS, attenuation, light_color);
      }

      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         const float intensity = light::intensity_spot(normalWS, positionWS, spot_light);
         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float3 light_dirWS;
         float attenuation;

         spot_params(positionWS, spot_light, attenuation, light_dirWS);

         light += brdf_light(params, light_dirWS, attenuation, light_color);
      }

      if (surface.use_ibl) {
         light += evaluate_ibl(params, surface.ao, surface.perceptual_roughness);
      }
      else {
         light += (light::ambient(normalWS) * params.diffuse_color *
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