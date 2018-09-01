
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "lighting_utilities_specular.hlsl"
#include "pixel_utilities.hlsl"
#include "normal_entrypoint_defs.hlsl"

// Samplers
sampler2D projected_light_texture : register(ps, s[2]);
sampler2D shadow_map : register(ps, s[3]);
sampler2D diffuse_map : register(ps, s[4]);
sampler2D normal_map : register(ps, s[5]);
sampler2D detail_map : register(ps, s[6]);
sampler2D detail_normal_map : register(ps, s[7]);

// Game Custom Constants
float4 blend_constant : register(ps, c[0]);
float4 shadow_blend : register(ps, c[1]);

float2 lighting_factor : register(c[CUSTOM_CONST_MIN]);
float4 texture_transforms[4] : register(vs, c[CUSTOM_CONST_MIN + 1]);

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

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD0;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 position : POSITION;

   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;
   
   float3 vertex_color : TEXCOORD6;
   float fade : TEXCOORD7;

   float fog_eye_distance : DEPTH;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;
    
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 normal = transform::normals(input.normals, input.blend_indices,
                                      input.weights);
   float3 world_normal = normals_to_world(normal);

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texture_transforms[0],
                                                             texture_transforms[1]);

   output.detail_texcoords = decompress_transform_texcoords(input.texcoords,
                                                           texture_transforms[2],
                                                           texture_transforms[3]);

   output.projection_texcoords = mul(world_position, light_proj_matrix);

   output.shadow_texcoords = transform_shadowmap_coords(world_position);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   output.vertex_color = input.color.rgb;
   output.fade = near_scene.fade * 0.25 + 0.5;

   return output;
}

void sample_normal_maps_gloss(float2 texcoords, float2 detail_texcoords, float3 world_normal,
                              float3 view_normal, out float3 out_normal, out float out_gloss)
{
   float4 N0_gloss = tex2D(normal_map, texcoords);
   float3 N0 = N0_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;
   N0.z = sqrt(1.0 - dot(N0.xy, N0.xy));

   if (use_detail_maps) {
      float3 N1 = tex2D(normal_map, detail_texcoords).xyz * 255.0 / 127.0 - 128.0 / 127.0;
      N1.z = sqrt(1.0 - dot(N1.xy, N1.xy));

      N0 = blend_tangent_space_normals(N0, N1);
   }

   out_normal = mul(N0, cotangent_frame(world_normal, -view_normal, texcoords));
   out_gloss = lerp(N0_gloss.a, 1.0, gloss_map_weight);
}

float3 do_lighting_diffuse(float3 world_normal, float3 world_position, float3 diffuse_color,
                           float3 static_diffuse_lighting, float shadow, 
                           float3 light_texture, Normal_state state)
{
   float4 diffuse_lighting = 0.0;
   diffuse_lighting.rgb = (light::ambient(world_normal) + static_diffuse_lighting) * diffuse_color;

   if (lighting_directional) {
      float4 intensity = float4(diffuse_lighting.rgb, 1.0);

      intensity.x = light::intensity_directional(world_normal, light_directional_0_dir);
      diffuse_lighting += intensity.x * light_directional_0_color;

      intensity.w = light::intensity_directional(world_normal, light_directional_1_dir);
      diffuse_lighting += intensity.w * light_directional_1_color;

      if (lighting_point_0) {
         intensity.y = light::intensity_point(world_normal, world_position, light_point_0_pos);
         diffuse_lighting += intensity.y * light_point_0_color;
      }

      if (lighting_point_1) {
         intensity.w = light::intensity_point(world_normal, world_position, light_point_1_pos);
         diffuse_lighting += intensity.w * light_point_1_color;
      }

      if (lighting_point_23) {
         intensity.w = light::intensity_point(world_normal, world_position, light_point_2_pos);
         diffuse_lighting += intensity.w * light_point_2_color;

         intensity.w = light::intensity_point(world_normal, world_position, light_point_3_pos);
         diffuse_lighting += intensity.w * light_point_3_color;
      }
      else if (lighting_spot_0) {
         intensity.z = light::intensity_spot(world_normal, world_position);
         diffuse_lighting += intensity.z * light_spot_color;
      }

      if (state.projected_tex) {
         float proj_intensity = dot(light_proj_selector, intensity);
         diffuse_lighting.rgb -= light_proj_color.rgb * proj_intensity;

         if (state.shadowed) {
            diffuse_lighting.rgb += light_proj_color.rgb * light_texture + proj_intensity;
         }
         else {
            float projection_shadow = lerp(1.0, shadow, shadow_blend.a);

            diffuse_lighting.rgb +=
               (light_proj_color.rgb * light_texture + proj_intensity) * projection_shadow;
         }
      }

      float3 color = diffuse_lighting.rgb * diffuse_color;

      float diffuse_intensity = diffuse_lighting.a;

      if (state.shadowed) {
         color *= (1.0 - (diffuse_intensity * (1.0 - shadow)));
      }

      float scale = max(color.r, color.g);
      scale = max(scale, color.b);
      scale = max(scale, 1.0);
      color = lerp(color / scale, color, tonemap_state);
      color *= hdr_info.z;

      return color;
   }
   else {
      return diffuse_color;
   }
}


float3 do_lighting(float3 world_normal, float3 world_position, float3 view_normal,
                   float3 diffuse_color, float3 static_diffuse_lighting, 
                   float3 specular_color, float shadow, float3 light_texture, 
                   Normal_state state)
{
   float3 color;
   color = (light::ambient(world_normal) + static_diffuse_lighting) * diffuse_color;

   float shadow_sum = 0.0;

   if (lighting_directional) {
      float4 intensity = float4(color, 1.0);

      color += light::specular::calculate(intensity.x, world_normal, view_normal,
                                          -light_directional_0_dir.xyz, light_directional_0_color.rgb,
                                          diffuse_color, specular_color, specular_exponent);
      shadow_sum += intensity.x * light_directional_0_color.a;
      
      color += light::specular::calculate(intensity.w, world_normal, view_normal,
                                          -light_directional_1_dir.xyz, light_directional_1_color.rgb,
                                          diffuse_color, specular_color, specular_exponent);
      shadow_sum += intensity.w * light_directional_1_color.a;

      if (lighting_point_0) {
         color += light::specular::calculate_point(intensity.y, world_normal, world_position, 
                                                   view_normal, light_point_0_pos, 
                                                   light_point_0_color.rgb, diffuse_color, 
                                                   specular_color, specular_exponent);
         shadow_sum += intensity.y * light_point_0_color.a;
      }

      if (lighting_point_1) {
         color += light::specular::calculate_point(intensity.w, world_normal, world_position, 
                                                   view_normal, light_point_1_pos, 
                                                   light_point_1_color.rgb, diffuse_color, 
                                                   specular_color, specular_exponent);
         shadow_sum += intensity.w * light_point_1_color.a;
      }

      if (lighting_point_23) {
         color += light::specular::calculate_point(intensity.w, world_normal, world_position, 
                                                   view_normal, light_point_2_pos, 
                                                   light_point_2_color.rgb, diffuse_color, 
                                                   specular_color, specular_exponent);
         shadow_sum += intensity.w * light_point_2_color.a;

         color += light::specular::calculate_point(intensity.w, world_normal, world_position, 
                                                   view_normal, light_point_3_pos, 
                                                   light_point_3_color.rgb, diffuse_color, 
                                                   specular_color, specular_exponent);
         shadow_sum += intensity.w * light_point_3_color.a;
      }
      else if (lighting_spot_0) {
         color += light::specular::calculate_spot(intensity.z, world_normal, view_normal,
                                                  world_position, diffuse_color, specular_color, 
                                                  specular_exponent);
         shadow_sum += intensity.z * light_spot_color.a;
      }

      if (state.projected_tex) {
         float proj_intensity = dot(light_proj_selector, intensity);
         color -= light_proj_color.rgb * proj_intensity;

         if (state.shadowed) {
            color += light_proj_color.rgb * light_texture + proj_intensity;
         }
         else {
            float projection_shadow = lerp(1.0, shadow, shadow_blend.a);

            color += (color * light_texture + proj_intensity) * projection_shadow;
         }
      }

      if (state.shadowed) {
         color *= saturate((1.0 - (shadow_sum * (1.0 - shadow))));
      }

      float scale = max(color.r, color.g);
      scale = max(scale, color.b);
      scale = max(scale, 1.0);
      color = lerp(color / scale, color, tonemap_state);
      color *= hdr_info.z;

      return color;
   }
   else {
      return diffuse_color;
   }
}


struct Ps_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;
   float3 vertex_color : TEXCOORD6;
   float fade : TEXCOORD7;

   float fog_eye_distance : DEPTH;

   float vface : VFACE;
};

float4 main_ps(Ps_input input, const Normal_state state) : COLOR
{

   float4 diffuse_map_color = tex2D(diffuse_map, input.diffuse_texcoords);

   // Hardedged Alpha Test
   if (state.hardedged && diffuse_map_color.a < 0.5) discard;

   const float3 light_texture = state.projected_tex ?
      sample_projected_light(projected_light_texture, input.projection_texcoords) : 1.0;
   const float shadow = 
      state.shadowed ? tex2Dproj(shadow_map, input.shadow_texcoords).a : 1.0;

   // Get Diffuse Color
   float3 diffuse_color = diffuse_map_color.rgb * base_diffuse_color;
   diffuse_color *= get_material_color(float4(input.vertex_color, 1.0)).rgb;

   if (use_detail_maps) {
      const float3 detail_color = tex2D(detail_map, input.detail_texcoords).rgb;
      diffuse_color = diffuse_color * detail_color * 2.0;
   }

   // Sample Normal Maps
   const float3 view_normal = normalize(world_view_position - input.world_position);

   float3 normal;
   float gloss;

   sample_normal_maps_gloss(input.diffuse_texcoords, input.detail_texcoords,
                            normalize(input.world_normal * -input.vface), 
                            view_normal, normal, gloss);


   float3 color;

   // Calculate Lighting
   float3 static_diffuse_lighting = get_static_diffuse_color(float4(input.vertex_color, 1.0));

   if (use_specular) {
      color = do_lighting(normal, input.world_position, view_normal, diffuse_color,
                          static_diffuse_lighting, base_specular_color * gloss,
                          shadow, light_texture, state);
   }
   else {
      color = do_lighting_diffuse(normal, input.world_position, diffuse_color, 
                                  static_diffuse_lighting, shadow, light_texture, state);
   }


   color = color * lighting_factor.x + lighting_factor.y;

   color = fog::apply(color.rgb, input.fog_eye_distance);

   float alpha = 1.0;

   if (use_transparency) {
      alpha = lerp(1.0, diffuse_map_color.a, blend_constant.b);
      alpha = saturate(alpha * input.fade);

      color /= alpha;
   }
   else {
      alpha = saturate(input.fade * 4.0);
   }

   return float4(color, alpha);
}

DEFINE_NORMAL_QPAQUE_PS_ENTRYPOINTS(Ps_input, main_ps)
DEFINE_NORMAL_TRANSPARENT_PS_ENTRYPOINTS(Ps_input, main_ps)