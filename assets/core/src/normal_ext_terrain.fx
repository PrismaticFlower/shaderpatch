
#include "color_utilities.hlsl"
#include "constants_list.hlsl"
#include "lighting_utilities.hlsl"
#include "normal_ext_common.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "terrain_common.hlsl"

// clang-format off

Texture2D<float3> projected_light_texture : register(t2);
Texture2D<float4> shadow_map : register(t3);
Texture2DArray<float1> height_maps : register(t7);
Texture2DArray<float4> diffuse_ao_maps : register(t8);
Texture2DArray<float3> normal_gloss_maps : register(t9);
TextureCube<float3> envmap : register(t10);

struct Texture_vars {
   float height_scale;
   float specular_exponent;
   uint2 padding;
};

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX) {
   float3 diffuse_color;
   float3 specular_color;
   bool use_envmap;

   float3x2 texture_transforms[16];

   Texture_vars texture_vars[16];
};

struct Vs_output {
   float3 positionWS : POSITIONWS;
   float fog : FOG;
   float3 normalWS : NORMALWS;
   float texture_blend0 : TEXBLEND0;
   float3 static_diffuse_lighting : STATICLIGHT;
   float texture_blend1 : TEXBLEND1;
   float fade : FADE;

   nointerpolation uint3 texture_indices : TEXTUREINDICES;

   float4 positionPS : SV_Position;
   float  cull_distance : SV_CullDistance;
};

Vs_output main_vs(Packed_terrain_vertex packed_vertex)
{
   Terrain_vertex input = unpack_vertex(packed_vertex, vertex_color_srgb);

   Vs_output output;

   output.positionWS = input.positionWS;
   output.normalWS = input.normalWS;
   output.texture_blend0 = input.texture_blend[0];
   output.texture_blend1 = input.texture_blend[1];
   output.texture_indices = input.texture_indices;
   output.static_diffuse_lighting = input.color;

   output.positionPS = mul(float4(output.positionWS, 1.0), projection_matrix);

   output.fog = calculate_fog(output.positionWS.y, output.positionPS.z);
   output.fade = calculate_near_fade(output.positionPS.z);

   output.cull_distance = 
      terrain_common_low_detail ? 
                                  calculate_prev_far_fade(output.positionPS.z * terrain_low_detail_cull_dist_mult) : 
                                  output.fade;

   return output;
}

struct Unpacked_textures {
   float3 diffuse;
   float ao;
   float3 normalTS;
   float gloss;
   float specular_exponent;
};

Unpacked_textures sample_textures(const Vs_output input, const float3x3 tangent_to_world)
{
   Unpacked_textures unpacked;

   const uint i0 = input.texture_indices[0];
   const uint i1 = input.texture_indices[1];
   const uint i2 = input.texture_indices[2];

   float3 texcoords[3] = {{mul(input.positionWS, texture_transforms[i0]), i0},
                          {mul(input.positionWS, texture_transforms[i1]), i1},
                          {mul(input.positionWS, texture_transforms[i2]), i2}};

   const float3 height_scales = {texture_vars[i0].height_scale,
                                 texture_vars[i1].height_scale,
                                 texture_vars[i2].height_scale};
   const float3 vertex_texture_blend = {input.texture_blend0, input.texture_blend1,
                                        1.0 - input.texture_blend0 - input.texture_blend1};
   const float3 unorm_viewTS = mul(view_positionWS - input.positionWS, transpose(tangent_to_world));

   float3 heights;

   terrain_sample_heightmaps(height_maps, height_scales, unorm_viewTS,
                             vertex_texture_blend, texcoords, heights);

   const float3 blend =
      terrain_blend_weights(heights, vertex_texture_blend);

   float4 diffuse_ao = 0.0;
   float3 normal_gloss = 0.0;

   [unroll] for (int i = 0; i < 3; ++i) {
      [branch] if (blend[i] > 0.0) {
         diffuse_ao +=
            diffuse_ao_maps.Sample(aniso_wrap_sampler, texcoords[i]) * blend[i];
         normal_gloss +=
            normal_gloss_maps.Sample(aniso_wrap_sampler, texcoords[i]) * blend[i];
      }
   }

   unpacked.diffuse = diffuse_ao.rgb;
   unpacked.ao = diffuse_ao.a;

   unpacked.normalTS.xy = normal_gloss.xy * (255.0 / 127.0) - (128.0 / 127.0);
   unpacked.normalTS.z =
      sqrt(1.0 - saturate(dot(unpacked.normalTS.xy, unpacked.normalTS.xy)));

   unpacked.gloss = normal_gloss.z;
   unpacked.specular_exponent = texture_vars[i0].specular_exponent * blend[0] +
                                texture_vars[i1].specular_exponent * blend[1] +
                                texture_vars[i2].specular_exponent * blend[2];

   return unpacked;
}

float4 main_ps(Vs_output input) : SV_Target0
{
   const float3x3 tangent_to_world =
      terrain_tangent_to_world(normalize(input.normalWS));

   const Unpacked_textures textures = sample_textures(input, tangent_to_world);

   const uint2 shadow_coords = (uint2)input.positionPS.xy;
   const float shadow =
      normal_ext_use_shadow_map ? shadow_map[shadow_coords].a : 1.0;
   const float3 static_diffuse_lighting = input.static_diffuse_lighting;
   const float3 diffuse = textures.diffuse * diffuse_color;
   const float3 specular = textures.gloss * specular_color;
   const float  specular_exponent = textures.specular_exponent;
   const float  ao = textures.ao;
   const float3 normalWS = normalize(mul(textures.normalTS, tangent_to_world));
   const float3 viewWS = normalize(view_positionWS - input.positionWS);

   float3 color = do_lighting(normalWS, input.positionWS, viewWS, diffuse,
                              static_diffuse_lighting, specular, specular_exponent,
                              shadow, ao, projected_light_texture);

   [branch] if (use_envmap) {
      const float3 env_coords = calculate_envmap_reflection(normalWS, viewWS);
      const float3 env = envmap.Sample(aniso_wrap_sampler, env_coords);
      const float roughness = light::specular_exp_to_roughness(specular_exponent);
      const float so =
         light::pbr::specular_occlusion(saturate(dot(normalWS, viewWS)), ao, roughness);

      color += (so * env * specular);
   }

   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}