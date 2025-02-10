
#include "color_utilities.hlsl"
#include "constants_list.hlsl"
#include "lighting_pbr.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "terrain_common.hlsl"

// clang-format off

const static bool pbr_terrain_use_shadow_map = PBR_TERRAIN_USE_SHADOW_MAP;

Texture2D<float2> shadow_ao_map : register(t3);
Texture2DArray<float1> height_maps : register(t7);
Texture2DArray<float4> albedo_ao_maps : register(t8);
Texture2DArray<float4> normal_mr_maps : register(t9);

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX) {
   float3 base_color;
   float base_metallicness;
   float base_roughness;

   float3x2 texture_transforms[16];

   float texture_height_scales[16];
};

struct Vs_output {
   float3 positionWS : POSITIONWS;
   float fog : FOG;
   float3 normalWS : NORMALWS;
   float texture_blend0 : TEXBLEND0;
   float3 static_lighting : STATICLIGHT;
   float texture_blend1 : TEXBLEND1;
   float fade : FADE;

   nointerpolation uint3 texture_indices : TEXTUREINDICES;

   float4 positionPS : SV_Position;
   float  cull_distance : SV_CullDistance;
};

Vs_output main_vs(Packed_terrain_vertex packed_vertex)
{
   Terrain_vertex input = unpack_vertex(packed_vertex, true);

   Vs_output output;

   output.positionWS = input.positionWS;
   output.normalWS = input.normalWS;
   output.texture_blend0 = input.texture_blend[0];
   output.texture_blend1 = input.texture_blend[1];
   output.texture_indices = input.texture_indices;
   output.static_lighting = input.color;

   output.positionPS = mul(float4(output.positionWS, 1.0), projection_matrix); 
   output.fade = calculate_near_fade(output.positionPS);
   output.fog = calculate_fog(output.positionWS, output.positionPS);
   output.cull_distance = 
      terrain_common_low_detail ? 
                                  terrain_far_scene_fade(output.positionPS.z * terrain_low_detail_cull_dist_mult) : 
                                  output.fade;


   return output;
}

struct Pbr_unpacked_textures {
   float3 albedo;
   float ao;
   float3 normalTS;
   float metallicness;
   float roughness;
};

Pbr_unpacked_textures sample_textures(const Vs_output input, const float3x3 tangent_to_world)
{
   Pbr_unpacked_textures unpacked;

   const uint i0 = input.texture_indices[0];
   const uint i1 = input.texture_indices[1];
   const uint i2 = input.texture_indices[2];

   float3 texcoords[3] = {{mul(input.positionWS, texture_transforms[i0]), i0},
                          {mul(input.positionWS, texture_transforms[i1]), i1},
                          {mul(input.positionWS, texture_transforms[i2]), i2}};

   const float3 height_scales = {texture_height_scales[i0],
                                 texture_height_scales[i1],
                                 texture_height_scales[i2]};
   const float3 vertex_texture_blend = {input.texture_blend0, input.texture_blend1,
                                        1.0 - input.texture_blend0 - input.texture_blend1};
   const float3 unorm_viewTS = mul(view_positionWS - input.positionWS, transpose(tangent_to_world));

   float3 heights;

   terrain_sample_heightmaps(height_maps, height_scales, unorm_viewTS,
                             vertex_texture_blend, texcoords, heights);

   const float3 blend =
      terrain_blend_weights(heights, vertex_texture_blend);

   float4 albedo_ao = 0.0;
   float4 normal_mr = 0.0;

   [unroll] for (int i = 0; i < 3; ++i) {
      [branch] if (blend[i] > 0.0) {
         albedo_ao +=
            albedo_ao_maps.Sample(aniso_wrap_sampler, texcoords[i]) * blend[i];
         normal_mr +=
            normal_mr_maps.Sample(aniso_wrap_sampler, texcoords[i]) * blend[i];
      }
   }

   unpacked.albedo = albedo_ao.rgb;
   unpacked.ao = albedo_ao.a;

   unpacked.normalTS.xy = normal_mr.xy * (255.0 / 127.0) - (128.0 / 127.0);
   unpacked.normalTS.z =
      sqrt(1.0 - saturate(dot(unpacked.normalTS.xy, unpacked.normalTS.xy)));

   unpacked.metallicness = normal_mr.z;
   unpacked.roughness = normal_mr.w;

   return unpacked;
}

float4 main_ps(Vs_output input) : SV_Target0
{
   const float3x3 tangent_to_world =
      terrain_tangent_to_world(normalize(input.normalWS));

   const Pbr_unpacked_textures textures = sample_textures(input, tangent_to_world);
   
   pbr::surface_info surface;

   const uint2 shadow_coords = (uint2)input.positionPS.xy;
   const float2 shadow_ao_sample = pbr_terrain_use_shadow_map  ? shadow_ao_map[shadow_coords] : float2(1.0, 1.0);

   surface.normalWS = normalize(mul(textures.normalTS, tangent_to_world));
   surface.viewWS = normalize(view_positionWS - input.positionWS); 
   surface.positionWS = input.positionWS;
   surface.metallicness = textures.metallicness * base_metallicness;
   surface.perceptual_roughness = textures.roughness * base_roughness;
   surface.base_color = textures.albedo * base_color;
   surface.sun_shadow = shadow_ao_sample.r;
   surface.ao = min(shadow_ao_sample.g, textures.ao);
   surface.use_ibl = false;

   float3 color = pbr::calculate(surface);

   color += (input.static_lighting * surface.ao * (surface.base_color / pbr::PI));

   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}