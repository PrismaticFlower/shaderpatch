#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "interior_mapping.hlsl"
#include "lighting_pbr.hlsl"
#include "normal_bf3_common.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float3>   projected_light_texture : register(ps, t2);
Texture2D<float2>   shadow_ao_map : register(ps, t3);
Texture2D<float4>   diffuse_map : register(ps, t7);
Texture2D<float3>   specular_map : register(ps, t8);
Texture2D<float4>   normal_map : register(ps, t9);
Texture2D<float>    ao_map : register(ps, t10);
Texture2D<float3>   emissive_map : register(ps, t11);
TextureCube<float3> env_map : register(ps, t12);
// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];

// Shader Feature Controls
const static bool use_texcoords_transform = NORMAL_BF3_USE_TEXCOORDS_TRANSFORM;
const static bool use_specular_map = NORMAL_BF3_USE_SPECULAR_MAP;
const static bool use_transparency = NORMAL_BF3_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_BF3_USE_HARDEDGED_TEST;

struct Vs_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

#if !NORMAL_BF3_USE_DYNAMIC_TANGENTS
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

# if !NORMAL_BF3_USE_DYNAMIC_TANGENTS
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

   if (use_transparency) {
      output.material_color_fade.rgb *= output.material_color_fade.a;
   }

   return output;
}

void get_normal_gloss(float2 texcoords, float3x3 tangent_to_world, out float3 out_normalWS, out float out_gloss)
{
   float3 normalTS;
   float gloss;

   normalTS = sample_normal_map_gloss(normal_map, aniso_wrap_sampler, texcoords, gloss);

   out_normalWS = mul(normalTS, tangent_to_world);
   out_gloss = gloss;
}

struct Ps_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

#if !NORMAL_BF3_USE_DYNAMIC_TANGENTS
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
#     if NORMAL_BF3_USE_DYNAMIC_TANGENTS
         return generate_tangent_to_world(_get_normalWS(), positionWS, texcoords);
#     else
         const float3 bitangentWS = bitangent_sign * cross(_get_normalWS(), tangentWS);

         return float3x3(tangentWS, bitangentWS, _get_normalWS());
#     endif
   }

   float3 _get_normalWS()
   {
#     if NORMAL_BF3_USE_DYNAMIC_TANGENTS
         const float3 aligned_normalWS = normalize((front_face ? -normalWS : normalWS) * dynamic_normal_sign);
#     else
         const float3 aligned_normalWS = front_face ? -normalWS : normalWS;
#     endif

      return aligned_normalWS;
   }

};

struct Ps_output {
   float4 out_color : SV_Target0;

#if NORMAL_BF3_USE_HARDEDGED_TEST
   uint out_coverage : SV_Coverage;
#endif

   void color(float4 color)
   {
      out_color = color;
   }

   void coverage(uint coverage)
   {  
#     if NORMAL_BF3_USE_HARDEDGED_TEST
         out_coverage = coverage;
#     endif
   }
};

Ps_output main_ps(Ps_input input)
{
   Ps_output output;

   const float3x3 tangent_to_world = input.tangent_to_world();

   const float3 viewWS = normalize(view_positionWS - input.positionWS);

   const float2 texcoords = input.texcoords;

   const float4 diffuse_map_color = 
      diffuse_map.Sample(aniso_wrap_sampler, texcoords);

   // Hardedged Alpha Test
   if (use_hardedged_test) {
      uint coverage = 0;

      [branch] 
      if (supersample_alpha_test) {
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
   const float3 diffuse_color = diffuse_map_color.rgb * base_diffuse_color * input.material_color_fade.rgb;

   // Sample Normal Maps
   float3 normalWS;
   float gloss;

   get_normal_gloss(texcoords, tangent_to_world, normalWS, gloss);

   // Apply gloss map weight.
   gloss = lerp(1.0, gloss, gloss_map_weight);

   float3 specular_color = base_specular_color ;

   if (use_specular_map) {
      specular_color *= specular_map.Sample(aniso_wrap_sampler, texcoords);
   }
   else { 
      specular_color *= gloss;
   }

   // Sample shadow map, if using.
   const float2 shadow_ao_sample = normal_bf3_use_shadow_map
                           ? shadow_ao_map[(int2)input.positionSS.xy]
                           : float2(1.0, 1.0);

   surface_info surface;
   
   surface.normalWS = normalWS;
   surface.positionWS = input.positionWS;
   surface.viewWS = viewWS;
   surface.diffuse_color = diffuse_color;
   surface.static_diffuse_lighting = input.static_lighting;
   surface.specular_color = specular_color;
   surface.shadow = shadow_ao_sample.r;
   surface.ao = use_ao_texture
                       ? min(ao_map.Sample(aniso_wrap_sampler, texcoords), shadow_ao_sample.g)
                       : shadow_ao_sample.g;

   // Calculate Lighting
   float3 color = do_lighting(surface, projected_light_texture);

   // Apply emissive map, if using.
   if (use_emissive_texture) {
      const float emissive_multiplier = emissive_power * lighting_scale;

      color += emissive_map.Sample(aniso_wrap_sampler, texcoords * emissive_texture_scale) * emissive_multiplier;
   }

   // Apply env map, if using.
   if (use_env_map) {
      const float3 env_coords = calculate_envmap_reflection(normalWS, viewWS);
      const float3 env = env_map.Sample(aniso_wrap_sampler, env_coords);
      const float roughness = light::specular_exp_to_roughness(specular_exponent);
      const float so = pbr::specular_occlusion(saturate(dot(normalWS, viewWS)), surface.ao, roughness);

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
   
   output.color(float4(color, alpha));

   return output;
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, uint coverage : SV_Coverage)
{
   Ps_output output = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, output.out_color, coverage);
}
