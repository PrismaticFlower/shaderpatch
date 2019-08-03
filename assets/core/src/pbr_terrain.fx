
#include "color_utilities.hlsl"
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "terrain_common.hlsl"
#include "lighting_utilities.hlsl"

Texture2D<float4> shadow_map : register(t3);
Texture2D<float3> terrain_normal_map : register(t6);
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
   float3 base_color : BASECOLOR;
   float texture_blend0 : TEXBLEND0;
   float2 terrain_coords : TERCOORDS;
   float texture_blend1 : TEXBLEND1;
   float fade : FADE;

   nointerpolation uint3 texture_indices : TEXTUREINDICES;

   float4 positionPS : SV_Position;
};

Vs_output main_vs(Packed_terrain_vertex packed_vertex)
{
   Terrain_vertex input = unpack_vertex(packed_vertex, true);

   Vs_output output;

   output.positionWS = input.positionWS;
   output.texture_blend0 = input.texture_blend[0];
   output.texture_blend1 = input.texture_blend[1];
   output.texture_indices = input.texture_indices;

   output.base_color = input.color * base_color;

   output.terrain_coords = input.terrain_coords;

   output.positionPS = mul(float4(output.positionWS, 1.0), projection_matrix);

   float fade;
   calculate_near_fade_and_fog(output.positionWS, output.positionPS, fade,
                               output.fog);
   output.fade = saturate(fade);

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
      terrain_sample_normal_map(terrain_normal_map, input.terrain_coords);

   const Pbr_unpacked_textures textures = sample_textures(input, tangent_to_world);

   const uint2 shadow_coords = (uint2)input.positionPS.xy;
   const float shadow = terrain_common_use_shadow_map ? shadow_map[shadow_coords].a : 1.0;
   const float3 albedo = textures.albedo * input.base_color;
   const float ao = textures.ao;
   const float metallicness = textures.metallicness * base_metallicness;
   const float roughness = textures.roughness * base_roughness;
   const float3 normalWS = normalize(mul(textures.normalTS, tangent_to_world));
   const float3 viewWS = normalize(view_positionWS - input.positionWS);

   float3 color = light::pbr::calculate(normalWS, viewWS, input.positionWS, 
                                        albedo, metallicness, roughness,
                                        ao, shadow);

   color = apply_fog(color, input.fog);

   return float4(color, input.fade);
}