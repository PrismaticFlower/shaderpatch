#include "adaptive_oit.hlsl" 
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "normal_ext_common.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

// Textures
Texture2D<float3> projected_light_texture : register(ps, t2);
Texture2D<float4> shadow_map : register(ps, t3);
Texture2D<float4> diffuse_map : register(ps, t6);
Texture2D<float4> normal_map : register(ps, t7);
Texture2D<float>  height_map : register(ps, t8);
Texture2D<float3> detail_map : register(ps, t9);
Texture2D<float2> detail_normal_map : register(ps, t10);
Texture2D<float3> emissive_map : register(ps, t11);
Texture2D<float4> overlay_diffuse_map : register(ps, t12);
Texture2D<float4> overlay_normal_map : register(ps, t13);

Texture2D<float>  displacement_map : register(ds, t0);

// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];
const static float  displacement_mip_interval = 32.0;
const static float  displacement_mip_base = 1024.0;
const static float  max_tess_factor = 48.0;

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float  disp_scale;
   float  disp_offset;
   float  material_tess_detail;
   float  tess_smoothing_amount;
   float3 base_diffuse_color;
   float  gloss_map_weight;
   float3 base_specular_color;
   float  specular_exponent;
   bool   use_parallax_occlusion_mapping;
   float  height_scale;
   bool   use_detail_textures;
   float  detail_texture_scale;
   bool   use_overlay_textures;
   float  overlay_texture_scale;
   bool   use_emissive_texture;
   float  emissive_texture_scale;
   float  emissive_power;
};

// Shader Feature Controls
const static bool use_texcoords_transform = NORMAL_EXT_USE_TEXCOORDS_TRANSFORM;
const static bool use_specular = NORMAL_EXT_USE_SPECULAR;
const static bool use_transparency = NORMAL_EXT_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_EXT_USE_HARDEDGED_TEST;
const static bool use_displacement_map = NORMAL_EXT_USE_DISPLACEMENT_MAP;

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

   float near_fade;
   calculate_near_fade_and_fog(positionWS, positionPS, near_fade, output.fog);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a = saturate(near_fade);
   output.static_lighting = get_static_diffuse_color(input.color());

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
         const float3 aligned_normalWS = normalize(front_face ? -normalWS : normalWS);
#     else
         const float3 aligned_normalWS = front_face ? -normalWS : normalWS;
#     endif

      return aligned_normalWS;
   }

};

float4 main_ps(Ps_input input) : SV_Target0
{
   const float3x3 tangent_to_world = input.tangent_to_world();

   const float3 view_normalWS = normalize(view_positionWS - input.positionWS);

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

   // Calculate Lighting
   if (use_specular) {
      color = do_lighting(normalWS, input.positionWS, view_normalWS, diffuse_color,
                          input.static_lighting, base_specular_color * gloss, specular_exponent,
                          shadow, 1.0, projected_light_texture);
   }
   else {
      color = do_lighting_diffuse(normalWS, input.positionWS, diffuse_color, 
                                  input.static_lighting, shadow, 1.0,
                                  projected_light_texture);
   }

   // Apply emissive map, if using.
   if (use_emissive_texture) {
      const float2 emissive_texcoords = input.texcoords * emissive_texture_scale;

      color += emissive_map.Sample(aniso_wrap_sampler, emissive_texcoords) * (emissive_power * lighting_scale);
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
      alpha = input.material_color_fade.a;
   }
   
   return float4(color, alpha);
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, uint coverage : SV_Coverage)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}

struct Hs_constant_output
{
   float edges[3] : SV_TessFactor;
   float inside : SV_InsideTessFactor;

#if NORMAL_EXT_USE_PN_TRIANGLES
   float3 b210 : B210;
   float3 b120 : B120;
   float3 b021 : B021;
   float3 b012 : B012;
   float3 b102 : B102;
   float3 b201 : B201;
   float3 b111 : B111;
#endif

};

Hs_constant_output constant_hs(InputPatch<Vs_output, 3> ip)
{
   Hs_constant_output output;

   const float tess_factor = material_tess_detail * tessellation_resolution_factor;

   [unroll] for (uint i = 0; i < 3; ++i) {
      const float3 edgeWS = ip[(i + 1) % 3].positionWS - ip[i].positionWS;
      const float3 vecWS = (ip[(i + 1) % 3].positionWS + ip[i].positionWS) / 2 - view_positionWS;
      const float lenWS = sqrt(dot(edgeWS, edgeWS) / dot(vecWS, vecWS));
      output.edges[(i + 1) % 3] = max(lenWS * tess_factor, 1.0);
   }

   output.inside = (output.edges[0] + output.edges[1] + output.edges[2]) / 3.0;
   
   bool cull_patch = false;
   
   [unroll] for (i = 0; i < 3; ++i) {
      const float2 positionSS = ip[i].positionPS.xy / ip[i].positionPS.w;

      cull_patch = cull_patch && (positionSS.x < -1.0 || positionSS.x > 1.0) && (positionSS.y < -1.0 || positionSS.y > 1.0);
   }
   
   if (cull_patch) {
      output.edges[0] = 0.0;
      output.edges[1] = 0.0;
      output.edges[2] = 0.0;
      output.inside = 0.0;
   }

#  if NORMAL_EXT_USE_PN_TRIANGLES
   {
      output.b210 = (2.0 * ip[2].positionWS + ip[0].positionWS - dot(ip[0].positionWS - ip[2].positionWS, ip[2].normalWS) * ip[2].normalWS) / 3.0;
      output.b120 = (2.0 * ip[0].positionWS + ip[2].positionWS - dot(ip[2].positionWS - ip[0].positionWS, ip[0].normalWS) * ip[0].normalWS) / 3.0;
      output.b021 = (2.0 * ip[0].positionWS + ip[1].positionWS - dot(ip[1].positionWS - ip[0].positionWS, ip[0].normalWS) * ip[0].normalWS) / 3.0;
      output.b012 = (2.0 * ip[1].positionWS + ip[0].positionWS - dot(ip[0].positionWS - ip[1].positionWS, ip[1].normalWS) * ip[1].normalWS) / 3.0;
      output.b102 = (2.0 * ip[1].positionWS + ip[2].positionWS - dot(ip[2].positionWS - ip[1].positionWS, ip[1].normalWS) * ip[1].normalWS) / 3.0;
      output.b201 = (2.0 * ip[2].positionWS + ip[1].positionWS - dot(ip[1].positionWS - ip[2].positionWS, ip[2].normalWS) * ip[2].normalWS) / 3.0;

      const float3 E = (output.b210 + output.b120 + output.b021 + output.b012 + output.b102 + output.b201) / 6.0;
      const float3 V = (ip[0].positionWS + ip[1].positionWS + ip[2].positionWS) / 3.0;

      output.b111 = E + (E - V) / 2.0;
   }
#  endif

   return output;
}

struct Hs_output
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
};

[domain("tri")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[patchconstantfunc("constant_hs")]
[outputcontrolpoints(3)]
[maxtessfactor(max_tess_factor)]
Hs_output main_hs(InputPatch<Vs_output, 3> ip,
                  uint id : SV_OutputControlPointID)
{
   Hs_output output;

   output.positionWS = ip[id].positionWS;
   output.normalWS = ip[id].normalWS;

#  if !NORMAL_EXT_USE_DYNAMIC_TANGENTS
      output.tangentWS = ip[id].tangentWS;
      output.bitangent_sign = ip[id].bitangent_sign;
#  endif

   output.texcoords = ip[id].texcoords;
   output.material_color_fade = ip[id].material_color_fade;
   output.static_lighting = ip[id].static_lighting;
   output.fog = ip[id].fog;

   return output;
}

#define INTERPOLATE_TRI(location, tri, attrib) \
   (tri[0].attrib * location.x) + (tri[1].attrib * location.y) + (tri[2].attrib * location.z)

[domain("tri")]
Vs_output main_ds(Hs_constant_output input,
                  float3 location : SV_DomainLocation,
                  const OutputPatch<Hs_output, 3> tri)
{
   Vs_output output;   
   
   const float2 texcoords = INTERPOLATE_TRI(location, tri, texcoords);
   const float3 normalWS = normalize(INTERPOLATE_TRI(location, tri, normalWS));
   float3 positionWS = INTERPOLATE_TRI(location, tri, positionWS);;
   
#  if NORMAL_EXT_USE_PN_TRIANGLES
   {
      const float u = location.x;
      const float v = location.y;
      const float w = location.z;
      const float u2 = u * u;
      const float v2 = v * v;
      const float w2 = w * w;
      const float u3 = u2 * u;
      const float v3 = v2 * v;
      const float w3 = w2 * w;

      const float3 new_posWS = tri[1].positionWS * v3 +
                               input.b102 * 3.0 * w * v2 + input.b012 * 3.0 * u * v2 +
                               input.b201 * 3.0 * w2 * v + input.b111 * 6.0 * w * u * v + input.b021 * 3.0 * u2 * v +
                               tri[2].positionWS * w3 + input.b210 * 3.0 * w2 * u + input.b120  * 3.0 * w * u2 + tri[0].positionWS * u3;

      positionWS = lerp(positionWS, new_posWS, tess_smoothing_amount);
   }
#  endif

   if (use_displacement_map) {
      const float displacement =
         displacement_map.SampleLevel(aniso_wrap_sampler, texcoords, 0) * disp_scale + disp_offset;

      positionWS += (normalWS * displacement);
   }

   output.positionWS = positionWS;
   output.normalWS = normalWS;

#  if !NORMAL_EXT_USE_DYNAMIC_TANGENTS
      output.tangentWS = normalize(INTERPOLATE_TRI(location, tri, tangentWS));
      output.bitangent_sign = INTERPOLATE_TRI(location, tri, bitangent_sign);
#  endif

   output.texcoords = texcoords;

   output.material_color_fade = INTERPOLATE_TRI(location, tri, material_color_fade);
   output.static_lighting = INTERPOLATE_TRI(location, tri, static_lighting);
   output.fog = INTERPOLATE_TRI(location, tri, fog);
   
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   return output;
}

#undef INTERPOLATE_TRI
