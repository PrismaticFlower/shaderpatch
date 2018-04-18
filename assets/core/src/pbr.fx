
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "normal_entrypoint_defs.hlsl"

// Samplers
sampler2D shadow_map : register(ps, s[3]);
sampler2D albedo_map : register(ps, s[4]);
sampler2D normal_map : register(ps, s[5]);
sampler2D metallic_roughness_map : register(ps, s[6]);
sampler2D ao_map : register(ps, s[7]);
sampler2D light_map : register(ps, s[8]);
sampler2D emissive_map : register(ps, s[9]);

// Game Custom Constants
float4 blend_constant : register(ps, c[0]);
float4 shadow_blend : register(ps, c[1]);

float2 lighting_factor : register(c[CUSTOM_CONST_MIN]);
float4 texture_transforms[4] : register(vs, c[CUSTOM_CONST_MIN + 1]);

// Material Constants Mappings
const static float3 base_color = material_constants[0].xyz;
const static float base_metallicness = material_constants[0].w;
const static float base_roughness = material_constants[1].x;
const static float ao_strength = material_constants[1].y;
const static float emissive_strength = material_constants[1].z;

// Shader Feature Controls
#ifdef PBR_USE_METALLIC_ROUGHNESS_MAP
const static bool use_metallic_roughness_map = true;
#else
const static bool use_metallic_roughness_map = false;
#endif

#ifdef PBR_USE_LIGHT_MAP
const static bool use_light_map = true;
#else
const static bool use_light_map = false;
#endif

#ifdef PBR_USE_EMISSIVE_MAP
const static bool use_emissive_map = true;
#else
const static bool use_emissive_map = false;
#endif

struct Vs_input
{
   float4 position : POSITION;
   float3 normal : NORMAL;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 position : POSITION;

   float2 texcoords : TEXCOORD0;
   float4 shadow_texcoords : TEXCOORD1;
   float3 world_position : TEXCOORD2;
   float3 world_normal : TEXCOORD3;

   float fade : COLOR0;

   float fog_eye_distance : DEPTH;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;
    
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 normal = transform::normals(input.normal, input.blend_indices,
                                      input.weights);
   float3 world_normal = normals_to_world(normal);

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);
   output.fade = calculate_near_scene_fade(world_position).fade * 0.25 + 0.5;

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   output.shadow_texcoords = transform_shadowmap_coords(world_position);

   return output;
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float4 shadow_texcoords : TEXCOORD1;
   float3 world_position : TEXCOORD2;
   float3 world_normal : TEXCOORD3;

   float fade : COLOR0;

   float fog_eye_distance : DEPTH;

   float vface : VFACE;
};

float4 main_opaque_ps(Ps_input input, const Normal_state state) : COLOR
{
   const float4 albedo_map_color = tex2D(albedo_map, input.texcoords);
   const float3 albedo = albedo_map_color.rgb * base_color;

   // Hardedged Alphatest
   if (state.hardedged && albedo_map_color.a < 0.5) discard;

   // Calculate Metallicness & Roughness factors
   float metallicness = base_metallicness;
   float roughness = base_roughness;

   if (use_metallic_roughness_map) {
      const float2 mr_map_color = tex2D(metallic_roughness_map, input.texcoords).xy;

      metallicness *= mr_map_color.x;
      roughness *= mr_map_color.y;
   }

   // Calculate lighting.
   const float3 V = normalize(world_view_position - input.world_position);
   const float3 N = perturb_normal(normal_map, input.texcoords,
                                   normalize(input.world_normal * sign(input.vface)), V);

   const float shadow =
      state.shadowed ? tex2Dproj(shadow_map, input.shadow_texcoords).a : 1.0;
   const float ao = tex2D(ao_map, input.texcoords).r * ao_strength;


   float3 color =
      light::pbr::calculate(N, V, input.world_position, albedo, metallicness, roughness, 
                            ao, shadow);

   if (use_emissive_map) {
      color += tex2D(emissive_map, input.texcoords).rgb * emissive_strength;
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, saturate(input.fade * 4.0));
}

float4 main_transparent_ps(Ps_input input, const Normal_state state) : COLOR
{
   const float4 albedo_map_color = tex2D(albedo_map, input.texcoords);

   if (state.hardedged && albedo_map_color.a < 0.5) discard;

   float4 color = main_opaque_ps(input, state);

   // TODO: Investigate usage of premultiplied alpha.

   return color;
}

DEFINE_NORMAL_QPAQUE_PS_ENTRYPOINTS(Ps_input, main_opaque_ps)
DEFINE_NORMAL_TRANSPARENT_PS_ENTRYPOINTS(Ps_input, main_transparent_ps)