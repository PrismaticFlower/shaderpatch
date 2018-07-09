
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "normal_entrypoint_defs.hlsl"

sampler2D detail_map : register(ps, s[1]);
sampler2D projected_texture : register(ps, s[2]);
sampler2D shadow_map : register(ps, s[3]);
sampler2D diffuse_map : register(ps, s[4]);
sampler2D light_map : register(ps, s[5]);

float4 blend_constant : register(ps, c[0]);
float4 shadow_blend : register(ps, c[1]);

float2 lighting_factor : register(c[CUSTOM_CONST_MIN]);
float4 texture_transforms[4] : register(vs, c[CUSTOM_CONST_MIN + 1]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
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

   float4 color : TEXCOORD6;

   float fog_eye_distance : DEPTH;
};

Vs_output near_opaque_shadow_projectedtex_vs(Vs_input input)
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

   output.color.rgb = get_material_color(input.color).rgb;
   output.color.a = near_scene.fade * 0.25 + 0.5;

   return output;
}

Vs_output near_transparent_shadow_projectedtex_vs(Vs_input input)
{
   Vs_output output = near_opaque_shadow_projectedtex_vs(input);
    
   Near_scene near_scene = calculate_near_scene_fade(float4(output.world_position, 1.0));

   near_scene = clamp_near_scene_fade(near_scene);

   float4 material_color = get_material_color(input.color);

   output.color.a = near_scene.fade * material_color.a;

   return output;
}

struct Ps_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 material_color : TEXCOORD6;

   float fog_eye_distance : DEPTH;
};

float4 main_opaque_ps(Ps_input input, const Normal_state state) : COLOR
{
   const float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   const float3 detail_color = tex2D(detail_map, input.detail_texcoords).rgb;
   const float3 projected_color = sample_projected_light(projected_texture, 
                                                         input.projection_texcoords);
   const float3 light_map_color = tex2D(light_map, input.diffuse_texcoords).rgb;
   const float shadow_map_color = tex2Dproj(shadow_map, input.shadow_texcoords).a;

   // Calculate lighting.
   Lighting light = light::calculate(normalize(input.world_normal), input.world_position,
                                     light_map_color);
   light.diffuse.rgb = light.diffuse.rgb * lighting_factor.x + lighting_factor.y;

   float4 color = 0.0;

   // Calculate projected texture lighting.
   if (state.projected_tex) {
      float3 projection_color;
      projection_color = light.static_diffuse.a * input.material_color.rgb;
      projection_color *= hdr_info.z;
      projection_color *= light_proj_color.rgb;

      if (state.shadowed) {
         float projection_shadow = lerp(1.0, shadow_map_color, shadow_blend.a);

         color.rgb = projected_color * projection_color * projection_shadow;
      }
      else {
         color.rgb = projected_color * projection_color;
      }
   }
     
   // Apply lighting to diffuse colour.
   color.rgb += light.diffuse.rgb;
   color.rgb = color.rgb * (diffuse_color.rgb * input.material_color.rgb);

   if (state.shadowed) {
      float shadow_value = light.diffuse.a * (1 - shadow_map_color);
       
      color.rgb *= (1 - shadow_value);
   }

   // Blend in detail texture.
   color.rgb = (color.rgb * detail_color) * 2.0;

   if (state.hardedged) {
      if (diffuse_color.a > 0.5) color.a = input.material_color.a - 0.5;
      else color.a = -0.01;

      color.a *= 4.0;
   }
   else {
      color.a = (input.material_color.a - 0.5) * 4.0;
   }

   color.a = saturate(color.a);

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 main_transparent_ps(Ps_input input, const Normal_state state) : COLOR
{
   const float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);

   float4 color = main_opaque_ps(input, state);

   if (state.hardedged) {
      if (diffuse_color.a > 0.5) color.a = (diffuse_color.a * input.material_color.a);
      else color.a = 0.0;
   }
   else {
      color.a = lerp(1.0, diffuse_color.a, blend_constant.b);
      color.a *= input.material_color.a;
   }

   color.a = saturate(color.a);

   return color;
}

DEFINE_NORMAL_QPAQUE_PS_ENTRYPOINTS(Ps_input, main_opaque_ps)
DEFINE_NORMAL_TRANSPARENT_PS_ENTRYPOINTS(Ps_input, main_transparent_ps)