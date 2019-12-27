#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

#pragma warning(disable : 3571)

// Samplers
Texture2D<float4> shadow_map : register(t3);
Texture2D<float4> albedo_map : register(t7);
Texture2D<float2> normal_map : register(t8);
Texture2D<float2> metallic_roughness_map : register(t9);
Texture2D<float> ao_map : register(t10);
Texture2D<float3> emissive_map : register(t11);

// Game Custom Constants
const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_texcoords_transform = custom_constants[1];
const static float4 y_texcoords_transform = custom_constants[2];

cbuffer MaterialConstants : register(b2)
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
const static bool use_shadow_map = PBR_USE_SHADOW_MAP;

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

float4 main_ps(Ps_input input) : SV_Target0
{
   const float4 albedo_map_color = albedo_map.Sample(aniso_wrap_sampler, input.texcoords);
   const float3 albedo = albedo_map_color.rgb * base_color;

   // Hardedged Alphatest
   if (use_hardedged_test && albedo_map_color.a < 0.5) discard;

   // Calculate Metallicness & Roughness factors
   float metallicness = base_metallicness;
   float roughness = base_roughness;

   if (use_metallic_roughness_map) {
      const float2 mr_map_color = metallic_roughness_map.Sample(aniso_wrap_sampler, input.texcoords);

      metallicness *= mr_map_color.x;
      roughness *= mr_map_color.y;
   }

   // Calculate lighting.
   float3 normalWS = input.front_face ? -input.normalWS : input.normalWS;
   const float3 bitangentWS = input.bitangent_sign * cross(normalWS, input.tangentWS);
   const float3x3 tangent_to_world = {input.tangentWS, bitangentWS, normalWS};

   const float3 normalTS = sample_normal_map(normal_map, aniso_wrap_sampler, input.texcoords);
   normalWS = normalize(mul(normalTS, tangent_to_world));

   const float shadow = 
      use_shadow_map ? shadow_map.SampleLevel(linear_clamp_sampler, input.shadow_texcoords, 0).a : 1.0;
   const float ao = ao_map.Sample(aniso_wrap_sampler, input.texcoords) * ao_strength;

   float3 color = light::pbr::calculate(normalWS, normalize(view_positionWS - input.positionWS), input.positionWS,
                                        albedo, metallicness, roughness, ao, shadow);

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

   return float4(color, alpha);
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, uint coverage : SV_Coverage)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}