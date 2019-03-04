#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

// Textures
Texture2D<float3> projected_light_texture : register(t2);
Texture2D<float4> shadow_map : register(t3);
Texture2D<float4> diffuse_map : register(t4);
Texture2D<float4> normal_map : register(t5);
Texture2D<float>  height_map : register(t6);
Texture2D<float3> detail_map : register(t7);
Texture2D<float2> detail_normal_map : register(t8);
Texture2D<float3> emissive_map : register(t9);
Texture2D<float4> overlay_diffuse_map : register(t10);
Texture2D<float4> overlay_normal_map : register(t11);

// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];

cbuffer MaterialConstants : register(b2)
{
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
const static bool use_specular = NORMAL_EXT_USE_SPECULAR;
const static bool use_transparency = NORMAL_EXT_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_EXT_USE_HARDEDGED_TEST;
const static bool use_shadow_map = NORMAL_EXT_USE_SHADOW_MAP;
const static bool use_projected_texture = NORMAL_EXT_USE_PROJECTED_TEXTURE;

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

   output.texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                            y_diffuse_texcoords_transform);

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

float3 do_lighting_diffuse(float3 normalWS, float3 positionWS, float3 diffuse_color,
                           float3 static_diffuse_lighting, float shadow)
{
   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(normalWS) + static_diffuse_lighting) * diffuse_color;

   if (ps_light_active_directional) {
      float3 proj_intensities = 0.0;
      float intensity;

      intensity = light::intensity_directional(normalWS, light_directional_0_dir);
      diffuse_lighting += intensity * light_directional_0_color;

      proj_intensities[0] = intensity;

      diffuse_lighting += light::intensity_directional(normalWS, light_directional_1_dir) * light_directional_1_color;

      if (ps_light_active_point_0) {
         intensity = light::intensity_point(normalWS, positionWS, light_point_0_pos, light_point_0_inv_range_sqr);
         diffuse_lighting += intensity * light_point_0_color;

         proj_intensities[1] = intensity;
      }

      if (ps_light_active_point_1) {
         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_1_pos, light_point_1_inv_range_sqr) * light_point_1_color;
      }

      if (ps_light_active_point_23) {
         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_2_pos, light_point_2_inv_range_sqr) * light_point_2_color;

         diffuse_lighting += 
            light::intensity_point(normalWS, positionWS, light_point_3_pos, light_point_3_inv_range_sqr) * light_point_3_color;
      }
      else if (ps_light_active_spot_light) {
         intensity = light::intensity_spot(normalWS, positionWS);
         diffuse_lighting += intensity * light_spot_color;

         proj_intensities[2] = intensity;
      }

      if (use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(positionWS, 1.0), light_proj_matrix));

         const float proj_intensity = dot(light_proj_selector.xyz, proj_intensities);
         diffuse_lighting.rgb -= light_proj_color.rgb * proj_intensity;
         diffuse_lighting.rgb += (light_proj_color.rgb * light_texture) * proj_intensity;
      }

      if (use_shadow_map) {
         const float shadow_mask = saturate(diffuse_lighting.a);

         diffuse_lighting.rgb *= (1.0 - (shadow_mask * (1.0 - shadow)));
      }

      float3 color = diffuse_lighting.rgb * diffuse_color;

      color *= lighting_scale;

      return color;
   }
   else {
      return diffuse_color * lighting_scale;
   }
}


float3 do_lighting(float3 normalWS, float3 positionWS, float3 view_normalWS,
                   float3 diffuse_color, float3 static_diffuse_lighting, 
                   float3 specular_color, float shadow)
{
   float4 diffuse_lighting = {light::ambient(normalWS) + static_diffuse_lighting, 0.0};
   float4 specular_lighting = 0.0;

   if (ps_light_active_directional) {
      float3 proj_intensities_diffuse = 0.0;
      float3 proj_intensities_specular = 0.0;

      light::blinnphong::calculate(diffuse_lighting, specular_lighting, normalWS, view_normalWS,
                                   -light_directional_0_dir.xyz, 1.0, light_directional_0_color, specular_exponent,
                                   proj_intensities_diffuse[0], proj_intensities_specular[0]);

      light::blinnphong::calculate(diffuse_lighting, specular_lighting, normalWS, view_normalWS, 
                                   -light_directional_0_dir.xyz, 1.0, light_directional_0_color, specular_exponent);

      if (ps_light_active_point_0) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_0_pos, 
                                            light_point_0_inv_range_sqr, light_point_0_color, specular_exponent,
                                            proj_intensities_diffuse[1], proj_intensities_specular[1]);
      }

      if (ps_light_active_point_1) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_1_pos, 
                                            light_point_1_inv_range_sqr, light_point_1_color,
                                            specular_exponent);
      }

      if (ps_light_active_point_23) {
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_2_pos, 
                                            light_point_2_inv_range_sqr, light_point_2_color,
                                            specular_exponent);
         
         light::blinnphong::calculate_point(diffuse_lighting, specular_lighting, normalWS,
                                            positionWS, view_normalWS, light_point_3_pos, 
                                            light_point_3_inv_range_sqr, light_point_3_color,
                                            specular_exponent);
      }
      else if (ps_light_active_spot_light) {
         light::blinnphong::calculate_spot(diffuse_lighting, specular_lighting, normalWS,
                                           view_normalWS, positionWS, specular_exponent,
                                           proj_intensities_diffuse[2], proj_intensities_specular[2]);
      }

      if (use_projected_texture) {
         const float3 light_texture = sample_projected_light(projected_light_texture, 
                                                             mul(float4(positionWS, 1.0), light_proj_matrix));

         const float3 masked_light_color = (light_proj_color.rgb * light_texture);

         const float diffuse_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_diffuse);

         diffuse_lighting.rgb -= (light_proj_color.rgb * diffuse_proj_intensity);
         diffuse_lighting.rgb += masked_light_color * diffuse_proj_intensity;

         const float specular_proj_intensity = dot(light_proj_selector.xyz, proj_intensities_specular);

         specular_lighting.rgb -= light_proj_color.rgb * specular_proj_intensity;
         specular_lighting.rgb += masked_light_color * specular_proj_intensity;
      }

      if (use_shadow_map) {
         const float shadow_diffuse_mask = saturate(diffuse_lighting.a);
         diffuse_lighting.rgb *= saturate((1.0 - (shadow_diffuse_mask * (1.0 - shadow))));

         const float shadow_specular_mask = saturate(specular_lighting.a);
         specular_lighting.rgb *= saturate((1.0 - (shadow_specular_mask * (1.0 - shadow))));
      }


      float3 color = 
         (diffuse_lighting.rgb * diffuse_color) + (specular_lighting.rgb * specular_color);

      color *= lighting_scale;

      return color;
   }
   else {
      return diffuse_color * lighting_scale;
   }
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
         const float3 bitangentWS = bitangent_sign * cross(normalWS, _get_normalWS());


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
      texcoords = parallax_occlusion_map(height_map, height_scale, input.texcoords,
                                         mul(view_positionWS - input.positionWS, transpose(tangent_to_world)),
                                         tangent_to_world[2], view_normalWS);
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
   const float shadow = use_shadow_map ? shadow_map[(int2)input.positionSS.xy].a : 1.0;

   // Calculate Lighting
   if (use_specular) {
      color = do_lighting(normalWS, input.positionWS, view_normalWS, diffuse_color,
                          input.static_lighting, base_specular_color * gloss,
                          shadow);
   }
   else {
      color = do_lighting_diffuse(normalWS, input.positionWS, diffuse_color, 
                                  input.static_lighting, shadow);
   }

   // Apply emissive map, if using.
   if (use_emissive_texture) {
      const float2 emissive_texcoords = input.texcoords * emissive_texture_scale;

      color += emissive_map.Sample(aniso_wrap_sampler, emissive_texcoords) * emissive_power;
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
