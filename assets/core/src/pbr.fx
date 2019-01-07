
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

#pragma warning(disable : 3571)

// Samplers
Texture2D<float4> shadow_map : register(t3);
Texture2D<float4> albedo_map : register(t4);
Texture2D<float2> normal_map : register(t5);
Texture2D<float2> metallic_roughness_map : register(t6);
Texture2D<float> ao_map : register(t7);
Texture2D<float4> emissive_map : register(t9);

// Game Custom Constants
const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_texcoords_transform = custom_constants[1];
const static float4 y_texcoords_transform = custom_constants[2];

// Material Constants Mappings
const static float3 base_color = material_constants[0].xyz;
const static float base_metallicness = material_constants[0].w;
const static float base_roughness = material_constants[1].x;
const static float ao_strength = material_constants[1].y;
const static float emissive_power = material_constants[1].z;

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
   float2 texcoords : TEXCOORD0;
   float4 shadow_texcoords : TEXCOORD1;

   float fade : FADE;
   float fog : FOG;

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

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   output.shadow_texcoords = transform_shadowmap_coords(positionWS);

   float near_fade;
   calculate_near_fade_and_fog(positionWS, positionPS, near_fade, output.fog);
   output.fade = saturate(near_fade);

   return output;
}

struct Ps_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float2 texcoords : TEXCOORD0;
   float4 shadow_texcoords : TEXCOORD1;

   float fade : FADE;
   float fog : FOG;

   float4 positionSS : SV_Position;
   float vface : VFACE;
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
   const float3 view_normalWS = normalize(view_positionWS - input.positionWS);
   const float3 normalWS = perturb_normal(normal_map, aniso_wrap_sampler, input.texcoords,
                                          normalize(input.normalWS * -input.vface), view_normalWS);

   const float2 shadow_texcoords = input.shadow_texcoords.xy / input.shadow_texcoords.w;
   const float shadow = 
      use_shadow_map ? shadow_map.SampleLevel(linear_clamp_sampler, shadow_texcoords, 0).a : 1.0;
   const float ao = ao_map.Sample(aniso_wrap_sampler, input.texcoords) * ao_strength;

   float3 color = light::pbr::calculate(normalWS, view_normalWS, input.positionWS, 
                                        albedo, metallicness, roughness, ao, shadow);

   if (use_emissive_map) {
      color += 
         emissive_map.Sample(aniso_wrap_sampler, input.texcoords).rgb * exp2(emissive_power);
   }

   color = apply_fog(color, input.fog);

   float alpha;

   if (use_transparency) {
      alpha = lerp(1.0, albedo_map_color.a, blend_constant.b);
      alpha *= input.fade;

      color /= alpha;
   }
   else {
      alpha = input.fade;
   }

   return float4(color, alpha);
}
