#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_brdf.hlsl"
#include "lighting_pbr.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float3> projected_light_texture : register(ps, t2);
Texture2D<float3> refraction_map : register(ps, t5);
Texture2D<float> depth_buffer : register(ps, t6);
Texture2D<float2> normal_map : PS_MATERIAL_REGISTER(0);
TextureCube<float3> reflection_map : PS_MATERIAL_REGISTER(1);

// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float3 refraction_color;
   float  refraction_scale;
   float3 reflection_color;
   float  small_bump_scale;
   float2 small_scroll;
   float2 medium_scroll;
   float2 large_scroll;
   float  medium_bump_scale;
   float  large_bump_scale;
   float  fresnel_min;
   float  fresnel_max;
   float  specular_exponent_dir_lights;
   float  specular_strength_dir_lights;
   float3 back_refraction_color;
   float  specular_exponent;
};

// Shader Static Constants
float2x2 tex_transform_matrix(float angle, float scale)
{
   return float2x2(cos(angle), -sin(angle), sin(angle), cos(angle)) * float2x2(scale, 0.0, 0.0, scale);
}

const static float medium_tex_angle = 1.570796;
const static float medium_tex_scale = 2.0;
const static float2x2 medium_tex_transform = tex_transform_matrix(medium_tex_angle, medium_tex_scale);

const static float large_tex_angle = 0.785398;
const static float large_tex_scale = 7.0;
const static float2x2 large_tex_transform = tex_transform_matrix(large_tex_angle, large_tex_scale);

// Shader Feature Controls
const static bool static_water_use_specular = STATIC_WATER_USE_SPECULAR;
const static bool sp_use_projected_texture = SP_USE_PROJECTED_TEXTURE;

struct Vs_output {
   float3 positionWS : POSITIONWS;
   float fade : FADE;
   float3 normalWS : NORMALWS;
   float fog : FOG;


   float3 tangentWS : TANGENTWS;
   float bitangent_sign : BITANGENTSIGN;

   float2 texcoords : TEXCOORDS;
   noperspective float2 refraction_texcoords : REFRACTTEXCOORDS;

   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();
   output.tangentWS = transformer.patch_tangentWS();
   output.bitangent_sign = input.patch_bitangent_sign();
   output.texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                            y_diffuse_texcoords_transform);
   output.refraction_texcoords = (output.positionPS.xy / output.positionPS.w) * float2(0.5, -0.5) + 0.5;
   output.fade = calculate_near_fade(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_input {
   float3 positionWS : POSITIONWS;
   float fade : FADE;
   float3 normalWS : NORMALWS;
   float fog : FOG;

   float3 tangentWS : TANGENTWS;
   float bitangent_sign : BITANGENTSIGN;

   float2 texcoords : TEXCOORDS;
   noperspective float2 refraction_texcoords : REFRACTTEXCOORDS;

   float4 positionSS : SV_Position;
   bool front_face : SV_IsFrontFace;
};

float3 sample_normal_map(float2 texcoords)
{
   const float2 sm_texcoords = texcoords + (time_seconds * small_scroll);
   const float2 md_texcoords = mul(texcoords + (time_seconds * medium_scroll), medium_tex_transform);
   const float2 lg_texcoords = mul(texcoords + (time_seconds * large_scroll), large_tex_transform);

   float3 sm_normal;
   sm_normal.xy = 
      (normal_map.Sample(aniso_wrap_sampler, sm_texcoords) * (255.0 / 127.0) - (128.0 / 127.0)) * small_bump_scale;
   sm_normal.z = sqrt(1.0 - saturate(dot(sm_normal.xy, sm_normal.xy)));
   float3 md_normal;
   md_normal.xy = 
      (normal_map.Sample(aniso_wrap_sampler, md_texcoords) * (255.0 / 127.0) - (128.0 / 127.0)) * medium_bump_scale;
   md_normal.z = sqrt(1.0 - saturate(dot(md_normal.xy, md_normal.xy)));
   float3 lg_normal;
   lg_normal.xy = 
      (normal_map.Sample(aniso_wrap_sampler, lg_texcoords) * (255.0 / 127.0) - (128.0 / 127.0)) * large_bump_scale;
   lg_normal.z = sqrt(1.0 - saturate(dot(lg_normal.xy, lg_normal.xy)));


   return normalize(float3(sm_normal.xy + md_normal.xy + lg_normal.xy, sm_normal.z * md_normal.z * lg_normal.z));
}

float3 sample_reflection_map(float3 reflectWS)
{
   const float3 reflection = reflection_map.Sample(aniso_wrap_sampler, reflectWS);

   return reflection * reflection_color * lighting_scale;
}

void sample_refraction_map(float2 texcoords, float3 normalTS, float depth,
                           out float3 refraction, out float3 back_refraction)
{
   const float2 disort_texcoords = texcoords + (refraction_scale * normalTS.xy);  
   const float4 scene_depth = depth_buffer.Gather(linear_mirror_sampler, disort_texcoords);
   const float2 final_texcoords = (all(scene_depth > depth)) ? disort_texcoords : texcoords;
   const float3 scene = refraction_map.SampleLevel(linear_mirror_sampler, final_texcoords, 0);

   refraction = scene * refraction_color;
   back_refraction = scene * back_refraction_color;
}

float3 get_specular(float3 normalWS, float3 viewWS, float3 positionWS, float4 positionSS);

float4 main_ps(Ps_input input) : SV_Target0
{
   const float3x3 tangent_to_world = {input.tangentWS,
                                      input.bitangent_sign * cross(input.normalWS, input.tangentWS),
                                      input.normalWS};

   const float3 normalTS = sample_normal_map(input.texcoords);
   const float3 normalWS = normalize(mul(normalTS, tangent_to_world));
   const float3 viewWS = normalize(view_positionWS - input.positionWS);
   const float3 reflectWS = normalize(reflect(-viewWS, normalWS));

   const float fresnel = clamp(pbr::F_schlick(0.05, normalWS, viewWS).x, fresnel_min, fresnel_max);

   const float3 reflection = sample_reflection_map(reflectWS);
   float3 refraction, back_refraction;
   sample_refraction_map(input.refraction_texcoords, normalTS, input.positionSS.z, 
                         refraction, back_refraction);

   float3 color = lerp(refraction, reflection, fresnel);

   if (static_water_use_specular) {
      color += get_specular(normalWS, viewWS, input.positionWS, input.positionSS);
   }

   [flatten]
   if (input.front_face) {
      color = back_refraction;
   }

   // Apply fog.
   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}

[earlydepthstencil] 
void oit_main_ps(Ps_input input, uint coverage : SV_Coverage) {
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color, coverage);
}

float3 get_specular(float3 normalWS, float3 viewWS, float3 positionWS, float4 positionSS)
{
   float4 specular = 0.0;
   float4 throwaway_diffuse = 0.0;
   
   [branch]
   if (light_active) {
      const float3 projected_light_texture_color = 
         sp_use_projected_texture ? sample_projected_light(projected_light_texture, 
                                                           mul(float4(positionWS, 1.0), light_proj_matrix)) 
                                  : 0.0;

      Lights_context context = acquire_lights_context(positionWS, positionSS);

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

         light::blinnphong::calculate(throwaway_diffuse, specular, normalWS, viewWS,
                                      -directional_light.directionWS, shadowing * specular_strength_dir_lights, 
                                      light_color, directional_light.stencil_shadow_factor(), 
                                      specular_exponent_dir_lights);
      }

      while (!context.point_lights_end()) {
         Point_light point_light = context.next_point_light();

         float3 light_color = point_light.color;

         if (point_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float3 light_dirWS;
         float attenuation;
      
         pbr::point_params(positionWS, point_light.positionWS, point_light.inv_range_sq, light_dirWS, attenuation);

         light::blinnphong::calculate(throwaway_diffuse, specular, normalWS, viewWS,
                                      light_dirWS, attenuation, light_color, 
                                      point_light.stencil_shadow_factor(), specular_exponent_dir_lights);
      }
      
      while (!context.spot_lights_end()) {
         Spot_light spot_light = context.next_spot_light();

         float3 light_color = spot_light.color;

         if (spot_light.use_projected_texture()) {
            light_color *= projected_light_texture_color;
         }

         float3 light_dirWS;
         float attenuation;

         pbr::spot_params(positionWS, spot_light, attenuation, light_dirWS);
         
         light::blinnphong::calculate(throwaway_diffuse, specular, normalWS, viewWS,
                                      light_dirWS, attenuation, light_color, 
                                      spot_light.stencil_shadow_factor(), specular_exponent_dir_lights);
      }
   }

   return specular.rgb * reflection_color * lighting_scale;
}

