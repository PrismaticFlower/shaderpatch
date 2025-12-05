#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_pbr.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

#pragma warning(disable : 3571)

// Samplers
Texture2D<float3> projected_light_texture : register(ps, t2);
Texture2D<float2> shadow_ao_map : register(t3);
Texture2D<float4> albedo_map : PS_MATERIAL_REGISTER(0);
Texture2D<float2> normal_map : PS_MATERIAL_REGISTER(1);
Texture2D<float2> metallic_roughness_map : PS_MATERIAL_REGISTER(2);
Texture2D<float> ao_map : PS_MATERIAL_REGISTER(3);
Texture2D<float3> emissive_map : PS_MATERIAL_REGISTER(4);

// Game Custom Constants
const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_texcoords_transform = custom_constants[1];
const static float4 y_texcoords_transform = custom_constants[2];

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float3 base_color;
   float base_metallicness;
   float base_roughness;
   float ao_strength;
   float emissive_power;
};

// Shader Feature Controls
const static bool use_metallic_roughness_map = PBR_USE_METALLIC_ROUGHNESS_MAP;
const static bool use_emissive_map = PBR_USE_EMISSIVE_MAP;
const static bool use_transparency = PBR_USE_TRANSPARENCY;
const static bool use_hardedged_test = PBR_USE_HARDEDGED_TEST;
const static bool use_shadow_map = SP_USE_STENCIL_SHADOW_MAP;
const static bool use_ibl = PBR_USE_IBL;

struct Vs_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float3 tangentWS : TANGENTWS;
   float2 texcoords : TEXCOORD0;
   noperspective float2 shadow_texcoords : TEXCOORD1;

   float fade : FADE;
   float fog : FOG;
   float bitangent_sign : BITANGENTSIGN;

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

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);
   output.fade =
      use_transparency ? calculate_near_fade_transparent(positionPS) : 
                         calculate_near_fade(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float3 tangentWS : TANGENTWS;
   float2 texcoords : TEXCOORD0;
   noperspective float2 shadow_texcoords : TEXCOORD1;

   float fade : FADE;
   float fog : FOG;
   float bitangent_sign : BITANGENTSIGN;

   float4 positionSS : SV_Position;
   bool front_face : SV_IsFrontFace;
};

struct Ps_output {
   float4 out_color : SV_Target0;

#if PBR_USE_HARDEDGED_TEST
   uint out_coverage : SV_Coverage;
#endif

   void color(float4 color)
   {
      out_color = color;
   }

   void coverage(uint coverage)
   {  
#     if PBR_USE_HARDEDGED_TEST
         out_coverage = coverage;
#     endif
   }
};

Ps_output main_ps(Ps_input input)
{
   Ps_output output;

   const float4 albedo_map_color = albedo_map.Sample(aniso_wrap_sampler, input.texcoords);
   const float3 albedo = albedo_map_color.rgb * base_color;

   // Hardedged Alphatest
   if (use_hardedged_test) {
      uint coverage = 0;

      [branch] 
      if (supersample_alpha_test) {
         for (uint i = 0; i < GetRenderTargetSampleCount(); ++i) {
            const float2 sample_texcoords = EvaluateAttributeAtSample(input.texcoords, i);
            const float alpha = albedo_map.Sample(aniso_wrap_sampler, sample_texcoords).a;

            const uint visible = alpha >= 0.5;

            coverage |= (visible << i);
         }
      }
      else {
         coverage = albedo_map_color.a >= 0.5 ? 0xffffffff : 0;
      }

      if (coverage == 0) discard;

      output.coverage(coverage);
   }

   // Calculate Metallicness & Roughness factors
   float metallicness = base_metallicness;
   float perceptual_roughness = base_roughness;

   if (use_metallic_roughness_map) {
      const float2 mr_map_color = metallic_roughness_map.Sample(aniso_wrap_sampler, input.texcoords);

      metallicness *= mr_map_color.x;
      perceptual_roughness *= mr_map_color.y;
   }

   // Calculate lighting.
   float3 normalWS = input.front_face ? -input.normalWS : input.normalWS;
   const float3 bitangentWS = input.bitangent_sign * cross(normalWS, input.tangentWS);
   const float3x3 tangent_to_world = {input.tangentWS, bitangentWS, normalWS};

   const float3 normalTS = sample_normal_map(normal_map, aniso_wrap_sampler, input.texcoords);
   normalWS = normalize(mul(normalTS, tangent_to_world));

   const float2 shadow_ao_sample = use_shadow_map
                           ? shadow_ao_map.Sample(linear_clamp_sampler, input.shadow_texcoords)
                           : float2(1.0, 1.0);
   const float shadow = shadow_ao_sample.r;
   const float ao = min(shadow_ao_sample.g, ao_map.Sample(aniso_wrap_sampler, input.texcoords) * ao_strength);

   pbr::surface_info surface;

   surface.normalWS = normalWS;
   surface.viewWS = normalize(view_positionWS - input.positionWS);
   surface.positionWS = input.positionWS;
   surface.base_color = albedo;
   surface.metallicness = metallicness;
   surface.perceptual_roughness = perceptual_roughness;
   surface.sun_shadow = shadow;
   surface.ao = ao;
   surface.positionSS = input.positionSS;
   surface.use_ibl = use_ibl;

   float3 color = pbr::calculate(surface, projected_light_texture);

   if (use_emissive_map) {
      color += 
         emissive_map.Sample(aniso_wrap_sampler, input.texcoords) * emissive_power;
   }

   color = apply_fog(color, input.fog);

   float alpha;

   if (use_transparency) {
      alpha = lerp(1.0, albedo_map_color.a, blend_constant.b);
      alpha *= input.fade;

      color /= max(alpha, 1e-5);
   }
   else {
      alpha = saturate(input.fade);
   }

   output.color(float4(color, alpha));

   return output;
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, uint coverage : SV_Coverage)
{
   Ps_output output = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, output.out_color, coverage);
}