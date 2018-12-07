
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "lighting_utilities_specular.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

// Samplers
Texture2D<float3> projected_light_texture : register(t2);
Texture2D<float4> shadow_map : register(t3);
Texture2D<float4> diffuse_map : register(t4);
Texture2D<float4> normal_map : register(t5);
Texture2D<float3> detail_map : register(t6);
Texture2D<float2> detail_normal_map : register(t7);

// Game Custom Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float2 lighting_factor = custom_constants[0].xy;
const static float4 x_diffuse_texcoords_transform = custom_constants[1];
const static float4 y_diffuse_texcoords_transform = custom_constants[2];
const static float4 x_detail_texcoords_transform  = custom_constants[3];
const static float4 y_detail_texcoords_transform  = custom_constants[4];

// Material Constants Mappings
const static float3 base_diffuse_color = material_constants[0].xyz;
const static float gloss_map_weight = material_constants[0].w;
const static float3 base_specular_color = material_constants[1].xyz;
const static float specular_exponent = material_constants[1].w;
const static float4 height_map_resolution = material_constants[2].x;
const static float height_scale = material_constants[3].x;

// Shader Feature Controls
const static bool use_detail_maps = NORMAL_EXT_USE_DETAIL_MAPS;
const static bool use_specular = NORMAL_EXT_USE_SPECULAR;
const static bool use_transparency = NORMAL_EXT_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_EXT_USE_HARDEDGED_TEST;
const static bool use_shadow_map = NORMAL_EXT_USE_SHADOW_MAP;
const static bool use_projected_texture = NORMAL_EXT_USE_PROJECTED_TEXTURE;

struct Vs_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 diffuse_texcoords : TEXCOORD2;
   float2 detail_texcoords : TEXCOORD3;
   float4 projection_texcoords : TEXCOORD4;
   float4 shadow_texcoords : TEXCOORD5;
   
   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog_eye_distance : DEPTH;

   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;
   
   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.diffuse_texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                                    y_diffuse_texcoords_transform);

   output.detail_texcoords = transformer.texcoords(x_detail_texcoords_transform,
                                                   y_detail_texcoords_transform);


   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_texcoords = transform_shadowmap_coords(positionWS);

   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a = saturate(near_scene.fade);
   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

void sample_normal_maps_gloss(float2 texcoords, float2 detail_texcoords, float3 normalWS,
                              float3 view_normalWS, out float3 out_normalWS, out float out_gloss)
{
   float4 N0_gloss = normal_map.Sample(aniso_wrap_sampler, texcoords);
   float3 N0 = N0_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;
   N0.z = sqrt(1.0 - dot(N0.xy, N0.xy));

   if (use_detail_maps) {
      float3 N1;
      N1.xy = detail_normal_map.Sample(aniso_wrap_sampler, detail_texcoords) * 255.0 / 127.0 - 128.0 / 127.0;
      N1.z = sqrt(1.0 - dot(N1.xy, N1.xy));

      N0 = blend_tangent_space_normals(N0, N1);
   }

   out_normalWS = mul(N0, cotangent_frame(normalWS, -view_normalWS, texcoords));
   out_gloss = lerp(N0_gloss.a, 1.0, gloss_map_weight);
}

float3 do_lighting_diffuse(float3 normalWS, float3 positionWS, float3 diffuse_color,
                           float3 static_diffuse_lighting, float shadow, 
                           float3 light_texture)
{
   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(normalWS) + static_diffuse_lighting) * diffuse_color;

   if (ps_light_active_directional) {
      float4 intensity = 0.0;

      intensity.x = light::intensity_directional(normalWS, light_directional_0_dir);
      diffuse_lighting += intensity.x * light_directional_0_color;

      intensity.w = light::intensity_directional(normalWS, light_directional_1_dir);
      diffuse_lighting += intensity.w * light_directional_1_color;

      if (ps_light_active_point_0) {
         intensity.y = light::intensity_point(normalWS, positionWS, light_point_0_pos);
         diffuse_lighting += intensity.y * light_point_0_color;
      }

      if (ps_light_active_point_1) {
         intensity.w = light::intensity_point(normalWS, positionWS, light_point_1_pos);
         diffuse_lighting += intensity.w * light_point_1_color;
      }

      if (ps_light_active_point_23) {
         intensity.w = light::intensity_point(normalWS, positionWS, light_point_2_pos);
         diffuse_lighting += intensity.w * light_point_2_color;

         intensity.w = light::intensity_point(normalWS, positionWS, light_point_3_pos);
         diffuse_lighting += intensity.w * light_point_3_color;
      }
      else if (ps_light_active_spot_light) {
         intensity.z = light::intensity_spot(normalWS, positionWS);
         diffuse_lighting += intensity.z * light_spot_color;
      }

      if (use_projected_texture) {
         const float proj_intensity = dot(light_proj_selector, intensity);
         diffuse_lighting.rgb -= light_proj_color.rgb * proj_intensity;
         diffuse_lighting.rgb += (light_proj_color.rgb * light_texture) * proj_intensity;
      }

      float3 color = diffuse_lighting.rgb * diffuse_color;

      float diffuse_intensity = diffuse_lighting.a;

      if (use_shadow_map) {
         color *= (1.0 - (diffuse_intensity * (1.0 - shadow)));
      }

      float scale = max(color.r, color.g);
      scale = max(scale, color.b);
      scale = max(scale, 1.0);
      color = lerp(color / scale, color, stock_tonemap_state);
      color *= lighting_scale;

      return color;
   }
   else {
      return diffuse_color * lighting_scale;
   }
}


float3 do_lighting(float3 normalWS, float3 positionWS, float3 view_normalWS,
                   float3 diffuse_color, float3 static_diffuse_lighting, 
                   float3 specular_color, float shadow, float3 light_texture)
{
   float3 diffuse_lighting = 0.0;
   float3 specular_lighting = 0.0;
   diffuse_lighting = (light::ambient(normalWS) + static_diffuse_lighting);

   float shadow_sum = 0.0;

   if (ps_light_active_directional) {
      float4 intensity_diffuse = 0.0;
      float4 intensity_specular = 0.0;

      light::specular::calculate(intensity_diffuse.x, intensity_specular.x, diffuse_lighting, 
                                 specular_lighting, normalWS, view_normalWS, 
                                 -light_directional_0_dir.xyz, 1.0, 
                                  light_directional_0_color.rgb, specular_exponent);
      shadow_sum += intensity_diffuse.x + intensity_specular.x * light_directional_0_color.a;
      
      light::specular::calculate(intensity_diffuse.w, intensity_specular.w, diffuse_lighting, 
                                 specular_lighting, normalWS, view_normalWS, 
                                 -light_directional_0_dir.xyz, 1.0,
                                 light_directional_0_color.rgb, specular_exponent);
      shadow_sum += intensity_diffuse.w + intensity_specular.w * light_directional_0_color.a;

      if (ps_light_active_point_0) {
         light::specular::calculate_point(intensity_diffuse.y, intensity_specular.y, 
                                          diffuse_lighting, specular_lighting, normalWS, 
                                          positionWS, view_normalWS, light_point_0_pos, 
                                          light_point_0_color.rgb, specular_exponent);
         shadow_sum += intensity_diffuse.y + intensity_specular.y * light_point_0_color.a;
      }

      if (ps_light_active_point_1) {
         light::specular::calculate_point(intensity_diffuse.w, intensity_specular.w, 
                                          diffuse_lighting, specular_lighting, normalWS, 
                                          positionWS, view_normalWS, light_point_1_pos, 
                                          light_point_1_color.rgb, specular_exponent);
         shadow_sum += intensity_diffuse.w + intensity_specular.w * light_point_1_color.a;
      }

      if (ps_light_active_point_23) {
         light::specular::calculate_point(intensity_diffuse.w, intensity_specular.w, 
                                          diffuse_lighting, specular_lighting, normalWS, 
                                          positionWS, view_normalWS, light_point_2_pos, 
                                          light_point_2_color.rgb, specular_exponent);
         shadow_sum += intensity_diffuse.w + intensity_specular.w * light_point_2_color.a;
         
         light::specular::calculate_point(intensity_diffuse.w, intensity_specular.w,
                                          diffuse_lighting, specular_lighting, normalWS, 
                                          positionWS, view_normalWS, light_point_3_pos, 
                                          light_point_3_color.rgb, specular_exponent);
         shadow_sum += intensity_diffuse.w + intensity_specular.w * light_point_3_color.a;
      }
      else if (ps_light_active_spot_light) {
         light::specular::calculate_spot(intensity_diffuse.z, intensity_specular.z, 
                                         diffuse_lighting, specular_lighting,
                                         normalWS, view_normalWS, positionWS, specular_exponent);
         shadow_sum += intensity_diffuse.z + intensity_specular.w * light_spot_color.a;
      }

      if (use_projected_texture) {
         const float diffuse_proj_intensity = dot(light_proj_selector, intensity_diffuse);

         diffuse_lighting -= (light_proj_color.rgb * diffuse_proj_intensity);
         diffuse_lighting += (light_proj_color.rgb * light_texture) * diffuse_proj_intensity;

         const float specular_proj_intensity = dot(light_proj_selector, intensity_specular);

         specular_lighting -= light_proj_color.rgb * specular_proj_intensity;
         specular_lighting += (light_proj_color.rgb * light_texture) * specular_proj_intensity;
      }

      float3 color = 
         (diffuse_lighting * diffuse_color) + (specular_lighting * specular_color);

      if (use_shadow_map) {
         color *= saturate((1.0 - (shadow_sum * (1.0 - shadow))));
      }

      float scale = max(color.r, color.g);
      scale = max(scale, color.b);
      scale = max(scale, 1.0);
      color = lerp(color / scale, color, stock_tonemap_state);
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

   float2 diffuse_texcoords : TEXCOORD2;
   float2 detail_texcoords : TEXCOORD3;
   float4 projection_texcoords : TEXCOORD4;
   float4 shadow_texcoords : TEXCOORD5;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog_eye_distance : DEPTH;

   float vface : VFACE;
};

float4 main_ps(Ps_input input) : SV_Target0
{
   const float4 diffuse_map_color = 
      diffuse_map.Sample(aniso_wrap_sampler, input.diffuse_texcoords);

   // Hardedged Alpha Test
   if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

   const float3 light_texture = use_projected_texture ?
      sample_projected_light(projected_light_texture, input.projection_texcoords) : 0.0;
   const float2 shadow_texcoords = input.shadow_texcoords.xy / input.shadow_texcoords.w;
   const float shadow = 
      use_shadow_map ? shadow_map.SampleLevel(linear_clamp_sampler, shadow_texcoords, 0).a : 1.0;

   // Get Diffuse Color
   float3 diffuse_color = diffuse_map_color.rgb * base_diffuse_color;
   diffuse_color *= input.material_color_fade.rgb;

   if (use_detail_maps) {
      const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, input.detail_texcoords);
      diffuse_color = diffuse_color * detail_color * 2.0;
   }

   // Sample Normal Maps
   const float3 view_normalWS = normalize(view_positionWS - input.positionWS);

   float3 normalWS;
   float gloss;

   sample_normal_maps_gloss(input.diffuse_texcoords, input.detail_texcoords,
                            normalize(input.normalWS * -input.vface), 
                            view_normalWS, normalWS, gloss);


   float3 color;

   // Calculate Lighting
   if (use_specular) {
      color = do_lighting(normalWS, input.positionWS, view_normalWS, diffuse_color,
                          input.static_lighting, base_specular_color * gloss,
                          shadow, light_texture);
   }
   else {
      color = do_lighting_diffuse(normalWS, input.positionWS, diffuse_color, 
                                  input.static_lighting, shadow, light_texture);
   }


   color = color * lighting_factor.x + lighting_factor.y;

   color = fog::apply(color.rgb, input.fog_eye_distance);

   float alpha = 1.0;

   if (use_transparency) {
      alpha = lerp(1.0, diffuse_map_color.a, blend_constant.b);
      alpha = saturate(alpha * input.material_color_fade.a);

      color /= alpha;
   }
   else {
      alpha = input.material_color_fade.a;
   }

   return float4(color, alpha);
}
