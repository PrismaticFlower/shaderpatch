#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "interior_mapping.hlsl"
#include "lighting_pbr.hlsl"
#include "normal_ext_common.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float3> projected_light_texture : register(ps, t2);
Texture2D<float2> shadow_ao_map : register(ps, t3);
Texture2D<float4> diffuse_map : register(ps, t7);
Texture2D<float4> specular_map : register(ps, t8);
Texture2D<float4> normal_map : register(ps, t9);
Texture2D<float>  height_map : register(ps, t10);
Texture2D<float3> detail_map : register(ps, t11);
Texture2D<float2> detail_normal_map : register(ps, t12);
Texture2D<float3> emissive_map : register(ps, t13);
Texture2D<float4> overlay_diffuse_map : register(ps, t14);
Texture2D<float4> overlay_normal_map : register(ps, t15);
Texture2D<float> ao_map : register(ps, t16);
TextureCube<float3> env_map : register(ps, t17);
TextureCubeArray<float3> interior_map_array : register(ps, t18);
// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];
const static float  displacement_mip_interval = 32.0;
const static float  displacement_mip_base = 1024.0;
const static float  max_tess_factor = 48.0;

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float3 base_diffuse_color;
   float  gloss_map_weight;
   float3 base_specular_color;
   float  base_specular_exponent;
   float  height_scale;
   bool   use_detail_textures;
   float  detail_texture_scale;
   bool   use_overlay_textures;
   float  overlay_texture_scale;
   bool   use_ao_texture;
   bool   use_emissive_texture;
   float  emissive_texture_scale;
   float  emissive_power;
   bool   use_env_map;
   float  env_map_vis;
   float  dynamic_normal_sign;
   float3 interior_spacing;
   uint   interior_hash_seed;
   float2 interior_map_array_size_info;
   bool   interior_randomize_walls;
};

// Shader Feature Controls
const static bool use_texcoords_transform = NORMAL_EXT_USE_TEXCOORDS_TRANSFORM;
const static bool use_specular = NORMAL_EXT_USE_SPECULAR;
const static bool use_specular_map = NORMAL_EXT_USE_SPECULAR_MAP;
const static bool use_specular_true_gloss = NORMAL_EXT_USE_SPECULAR_TRUE_GLOSS;
const static bool use_specular_normalized = NORMAL_EXT_USE_SPECULAR_NORMALIZED;
const static bool use_parallax_occlusion_mapping = NORMAL_EXT_USE_PARALLAX_OCCLUSION_MAPPING;
const static bool use_vertex_color_for_emissive = NORMAL_EXT_USE_VERTEX_COLOR_FOR_EMISSIVE;
const static bool use_transparency = NORMAL_EXT_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_EXT_USE_HARDEDGED_TEST;
const static bool use_interior_mapping = NORMAL_EXT_USE_INTERIOR_MAPPING;

struct Vs_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

#if !NORMAL_EXT_USE_DYNAMIC_TANGENTS
   float3 tangentWS : TANGENTWS;
   float  bitangent_sign : BITANGENTSIGN;
#endif

   float2 texcoords : TEXCOORDS;
   
   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;
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

# if !NORMAL_EXT_USE_DYNAMIC_TANGENTS
      output.tangentWS = transformer.patch_tangentWS();
      output.bitangent_sign = input.patch_bitangent_sign();
#  endif

   if (use_texcoords_transform) 
      output.texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                               y_diffuse_texcoords_transform);   
   else
      output.texcoords = input.texcoords();

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *=
      use_transparency ? calculate_near_fade_transparent(positionPS) : 
                         calculate_near_fade(positionPS);

   if (use_transparency) {
      output.material_color_fade.rgb *= output.material_color_fade.a;
   }

   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

void get_normal_gloss(float2 texcoords, float2 detail_texcoords, float3x3 tangent_to_world, out float3 out_normalWS,
                      out float out_gloss)
{
   float3 normalTS;
   float gloss;

   if (use_detail_textures) {
      normalTS = sample_normal_map_gloss(normal_map, detail_normal_map, aniso_wrap_sampler,
                                         texcoords, detail_texcoords, gloss);
   }
   else {
      normalTS = sample_normal_map_gloss(normal_map, aniso_wrap_sampler, texcoords, gloss);
   }

   out_normalWS = mul(normalTS, tangent_to_world);
   out_gloss = gloss;
}

struct Ps_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

#if !NORMAL_EXT_USE_DYNAMIC_TANGENTS
   float3 tangentWS : TANGENTWS;
   float  bitangent_sign : BITANGENTSIGN;
#endif

   float2 texcoords : TEXCOORDS;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionSS : SV_Position;
   bool front_face : SV_IsFrontFace;

   float3x3 tangent_to_world()
   {
#     if NORMAL_EXT_USE_DYNAMIC_TANGENTS
         return generate_tangent_to_world(_get_normalWS(), positionWS, texcoords);
#     else
         const float3 bitangentWS = bitangent_sign * cross(_get_normalWS(), tangentWS);

         return float3x3(tangentWS, bitangentWS, _get_normalWS());
#     endif
   }

   float3 _get_normalWS()
   {
#     if NORMAL_EXT_USE_DYNAMIC_TANGENTS
         const float3 aligned_normalWS = normalize((front_face ? -normalWS : normalWS) * dynamic_normal_sign);
#     else
         const float3 aligned_normalWS = front_face ? -normalWS : normalWS;
#     endif

      return aligned_normalWS;
   }

};

struct Ps_output {
   float4 out_color : SV_Target0;

#if NORMAL_EXT_USE_HARDEDGED_TEST
   uint out_coverage : SV_Coverage;
#endif

   void color(float4 color)
   {
      out_color = color;
   }

   void coverage(uint coverage)
   {  
#     if NORMAL_EXT_USE_HARDEDGED_TEST
         out_coverage = coverage;
#     endif
   }
};

Ps_output main_ps(Ps_input input)
{
   Ps_output output;

   const float3x3 tangent_to_world = input.tangent_to_world();

   const float3 viewWS = normalize(view_positionWS - input.positionWS);

   // Apply POM to texcoords if using, else just use interpolated texcoords.
   float2 texcoords;
   
   if (use_parallax_occlusion_mapping) {
      class Parallax_texture : Parallax_input_texture {
         float CalculateLevelOfDetail(SamplerState samp, float2 texcoords)
         {
            return height_map.CalculateLevelOfDetail(samp, texcoords);
         }

         float SampleLevel(SamplerState samp, float2 texcoords, float mip)
         {
            return height_map.SampleLevel(samp, texcoords, mip);
         }

         float Sample(SamplerState samp, float2 texcoords)
         {
            return height_map.Sample(samp, texcoords);
         }

         Texture2D<float> height_map;
      };

      Parallax_texture parallax_texture;
      parallax_texture.height_map = height_map;

      texcoords = parallax_occlusion_map(parallax_texture, height_scale, input.texcoords,
                                         mul(view_positionWS - input.positionWS, transpose(tangent_to_world)));
   }
   else {
      texcoords = input.texcoords;
   }

   const float4 diffuse_map_color = 
      diffuse_map.Sample(aniso_wrap_sampler, texcoords);

   // Hardedged Alpha Test
   if (use_hardedged_test) {
      uint coverage = 0;

      [branch] 
      if (!use_parallax_occlusion_mapping && supersample_alpha_test) {
         for (uint i = 0; i < GetRenderTargetSampleCount(); ++i) {
            const float2 sample_texcoords = EvaluateAttributeAtSample(input.texcoords, i);
            const float alpha = diffuse_map.Sample(aniso_wrap_sampler, sample_texcoords).a;

            const uint visible = alpha >= 0.5;

            coverage |= (visible << i);
         }
      }
      else {
         coverage = diffuse_map_color.a >= 0.5 ? 0xffffffff : 0;
      }

      if (coverage == 0) discard;

      output.coverage(coverage);
   }

   // Get Diffuse Color
   float3 diffuse_color = diffuse_map_color.rgb * base_diffuse_color;

   if (!use_vertex_color_for_emissive) {
      diffuse_color *= input.material_color_fade.rgb;
   }

   const float2 detail_texcoords = input.texcoords * detail_texture_scale;

   if (use_detail_textures) {
      const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, detail_texcoords);
      diffuse_color = diffuse_color * detail_color * 2.0;
   }

   // Sample Normal Maps
   float3 normalWS;
   float gloss;

   get_normal_gloss(texcoords, detail_texcoords, tangent_to_world, normalWS, gloss);

   // Apply overlay maps, if using.
   if (use_overlay_textures) {
      const float2 overlay_texcoords = input.texcoords * overlay_texture_scale;
      const float4 overlay_color = overlay_diffuse_map.Sample(aniso_wrap_sampler, overlay_texcoords);
      const float overlay_normal_map_mip = overlay_normal_map.CalculateLevelOfDetail(aniso_wrap_sampler, overlay_texcoords);

      [branch] if (overlay_color.a > 0.5) {
         float overlay_gloss;
         const float3 overlay_normalTS = sample_normal_map_gloss(overlay_normal_map, aniso_wrap_sampler, overlay_normal_map_mip,
                                                                 overlay_texcoords, overlay_gloss);

         normalWS = normalize(mul(overlay_normalTS, tangent_to_world));

         diffuse_color = (diffuse_color * (1.0 - overlay_color.a)) + overlay_color.rgb;
         gloss = lerp(gloss, overlay_gloss, overlay_color.a);
      }
   }

   // Apply gloss map weight.
   gloss = lerp(1.0, gloss, gloss_map_weight);

   float3 color;

   // Sample shadow map, if using.
   const float2 shadow_ao_sample = normal_ext_use_shadow_map
                           ? shadow_ao_map[(int2)input.positionSS.xy]
                           : float2(1.0, 1.0);
   const float shadow = shadow_ao_sample.r;
   const float ao = use_ao_texture
                       ? min(ao_map.Sample(aniso_wrap_sampler, texcoords), shadow_ao_sample.g)
                       : shadow_ao_sample.g;
   float specular_exponent = base_specular_exponent;

   // Calculate Lighting
   if (use_specular) {
      float3 specular_color = base_specular_color;

      if (use_specular_map) {
         const float4 specular_map_color = specular_map.Sample(aniso_wrap_sampler, texcoords);

         specular_color *= specular_map_color.rgb;
         gloss = specular_map_color.a;
      }
      else {
         specular_color *= gloss;
      }

      if (use_specular_true_gloss) {
         // Christian Schüler's Glossniess Anti-Aliasing - http://www.thetenthplanet.de/archives/3401
         const float3 ddx_normalWS = ddx(normalWS);
         const float3 ddy_normalWS = ddy(normalWS);
         const float curvature_sq = max(dot(ddx_normalWS, ddx_normalWS), dot(ddy_normalWS, ddy_normalWS));
         const float max_gloss = -0.0909 - 0.0909 * log2(0.5 * curvature_sq);

         gloss = min(gloss, max_gloss);

         specular_exponent = exp2(1.0 + gloss * 11.0);
      }

      if (use_specular_normalized) {
         color = do_lighting_normalized(normalWS, input.positionWS, viewWS, diffuse_color,
                                        input.static_lighting, specular_color, specular_exponent,
                                        shadow, ao, projected_light_texture);
      }
      else {
         color = do_lighting(normalWS, input.positionWS, viewWS, diffuse_color,
                             input.static_lighting, specular_color, specular_exponent,
                             shadow, ao, projected_light_texture);
      }
   }
   else {
      color = do_lighting_diffuse(normalWS, input.positionWS, diffuse_color, 
                                  input.static_lighting, shadow, ao,
                                  projected_light_texture);
   }

   // Apply emissive map, if using.
   if (use_emissive_texture) {
      const float2 emissive_texcoords =
         (use_parallax_occlusion_mapping && emissive_texture_scale == 1.0)
            ? texcoords
            : input.texcoords * emissive_texture_scale;
      const float3 emissive_vertex_color = 
         use_vertex_color_for_emissive ? input.material_color_fade.rgb : float3(1.0, 1.0, 1.0);
      const float3 emissive_base_color = emissive_vertex_color * emissive_power * lighting_scale;

      color += emissive_map.Sample(aniso_wrap_sampler, emissive_texcoords) * emissive_base_color;
   }

   // Apply env map, if using.
   if (use_specular && use_env_map) {
      const float3 env_coords = calculate_envmap_reflection(normalWS, viewWS);
      const float3 env = env_map.Sample(aniso_wrap_sampler, env_coords);
      const float roughness = light::specular_exp_to_roughness(specular_exponent);
      const float so = pbr::specular_occlusion(saturate(dot(normalWS, viewWS)), ao, roughness);

      color += (gloss * so * env_map_vis * env * base_specular_color);
   }

   // Apply interior mapping, if using.
   if (use_interior_mapping && diffuse_map_color.a < 1.0) {
      const float3 interior_color = 
         interior_map(normalize(mul(tangent_to_world, input.positionWS - view_positionWS)), 
                      input.texcoords, interior_spacing, interior_hash_seed, interior_randomize_walls, 
                      interior_map_array, interior_map_array_size_info);

      color.rgb = (interior_color * (1.0 - diffuse_map_color.a)) + color.rgb;
   }

   // Apply fog.
   color = apply_fog(color, input.fog);

   float alpha = 1.0;

   if (use_transparency) {
      alpha = lerp(1.0, diffuse_map_color.a, blend_constant.b);
      alpha = saturate(alpha * input.material_color_fade.a);

      color /= max(alpha, 1e-5);
   }
   else {
      alpha = saturate(input.material_color_fade.a);
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
