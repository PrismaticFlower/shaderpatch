#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_pbr.hlsl"
#include "normal_ext_common.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float3> projected_light_texture : register(ps, t2);
Texture2D<float4> shadow_map : register(ps, t3);
Texture2D<float4> diffuse_map : register(ps, t7);
Texture2D<float4> normal_map : register(ps, t8);
Texture2D<float>  height_map : register(ps, t9);
Texture2D<float3> detail_map : register(ps, t10);
Texture2D<float2> detail_normal_map : register(ps, t11);
Texture2D<float3> emissive_map : register(ps, t12);
Texture2D<float4> overlay_diffuse_map : register(ps, t13);
Texture2D<float4> overlay_normal_map : register(ps, t14);
Texture2D<float> ao_map : register(ps, t15);
TextureCube<float3> env_map : register(ps, t16);

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
   float  specular_exponent;
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
};

// Shader Feature Controls
const static bool use_texcoords_transform = NORMAL_EXT_USE_TEXCOORDS_TRANSFORM;
const static bool use_specular = NORMAL_EXT_USE_SPECULAR;
const static bool use_parallax_occlusion_mapping = NORMAL_EXT_USE_PARALLAX_OCCLUSION_MAPPING;
const static bool use_transparency = NORMAL_EXT_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_EXT_USE_HARDEDGED_TEST;

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

float4 main_ps(Ps_input input) : SV_Target0
{
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
   if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

   // Get Diffuse Color
   float3 diffuse_color = diffuse_map_color.rgb * base_diffuse_color;
   diffuse_color *= input.material_color_fade.rgb;

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
   const float shadow = normal_ext_use_shadow_map
                           ? shadow_map[(int2)input.positionSS.xy].a
                           : 1.0;
   const float ao = use_ao_texture
                       ? ao_map.Sample(aniso_wrap_sampler, texcoords)
                       : 1.0;

   // Calculate Lighting
   if (use_specular) {
      color = do_lighting(normalWS, input.positionWS, viewWS, diffuse_color,
                          input.static_lighting, base_specular_color * gloss, specular_exponent,
                          shadow, ao, projected_light_texture);
   }
   else {
      color = do_lighting_diffuse(normalWS, input.positionWS, diffuse_color, 
                                  input.static_lighting, shadow, ao,
                                  projected_light_texture);
   }

   // Apply emissive map, if using.
   if (use_emissive_texture) {
      const float2 emissive_texcoords = input.texcoords * emissive_texture_scale;

      color += emissive_map.Sample(aniso_wrap_sampler, emissive_texcoords) * (emissive_power * lighting_scale);
   }

   // Apply env map, if using.
   if (use_specular && use_env_map) {
      const float3 env_coords = calculate_envmap_reflection(normalWS, viewWS);
      const float3 env = env_map.Sample(aniso_wrap_sampler, env_coords);
      const float roughness = light::specular_exp_to_roughness(specular_exponent);
      const float so = pbr::specular_occlusion(saturate(dot(normalWS, viewWS)), ao, roughness);

      color += (gloss * so * env_map_vis * env * base_specular_color);
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
   
   return float4(color, alpha);
}

[earlydepthstencil]
void oit_main_ps(Ps_input input)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}
