
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "transform_utilities.hlsl"

// Disbale forced loop unroll warning.
#pragma warning(disable : 3550)

float4 terrain_constant : register(c[CUSTOM_CONST_MIN]);
float4 texture_coords[8] : register(vs, c[CUSTOM_CONST_MIN + 1]);

struct Vs_input
{
   float4 position : POSITION;
   float4 normal : NORMAL;
   float4 color : COLOR;
};

struct Vs_blendmap_output
{
   float4 position : POSITION;
   float2 texcoords[4] : TEXCOORD0;
   float3 blend_values : TEXCOORD4;
   float3 world_position : TEXCOORD5;
   float3 world_normal : TEXCOORD6;

   float3 precalculated_light : COLOR0;
   float fade : COLOR1;
   float fog_eye_distance : DEPTH;
};

Vs_blendmap_output diffuse_blendmap_vs(Vs_input input)
{
   Vs_blendmap_output output;

   float4 world_position = transform::position(input.position);
   float3 world_normal = normals_to_world(decompress_normals(input.normal.xyz));

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   for (int i = 0; i < 4; ++i) {
      output.texcoords[i].x = dot(world_position, texture_coords[0 + (i * 2)]);
      output.texcoords[i].y = dot(world_position, texture_coords[1 + (i * 2)]);
   }

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fade = near_scene.fade * 0.25 + 0.5;

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   output.precalculated_light = lighting.diffuse.rgb;

   output.blend_values.r = lighting.diffuse.a;
   output.blend_values.g = input.position.w * terrain_constant.w;
   output.blend_values.b = input.normal.w;

   return output;
}

struct Vs_detail_output
{
   float4 position : POSITION;
   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_map_texcoords : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float3 precalculated_light : COLOR0;
   float3 projection_color : COLOR1;
   float fade : COLOR2;

   float fog_eye_distance : DEPTH;
};

Vs_detail_output detailing_vs(Vs_input input)
{
   Vs_detail_output output;

   float4 world_position = transform::position(input.position);
   float3 world_normal = normals_to_world(decompress_normals(input.normal.xyz));

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   for (int i = 0; i < 2; ++i) {
      output.detail_texcoords[i].x = dot(world_position, texture_coords[0 + (i * 2)]);
      output.detail_texcoords[i].y = dot(world_position, texture_coords[1 + (i * 2)]);
   }

   output.projection_texcoords = mul(world_position, light_proj_matrix);
   output.shadow_map_texcoords = transform_shadowmap_coords(world_position); 

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   output.precalculated_light = lighting.diffuse.rgb;

   float4 material_color = get_material_color(input.color);

   output.projection_color = material_color.rgb * lighting.static_diffuse.a;
   output.projection_color *= light_proj_color.rgb;

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fade = saturate(near_scene.fade * 0.25 + 0.5);

   return output;
}

struct Ps_blendmap_input
{
   float2 texcoords[3] : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD3;
   float3 blend_values : TEXCOORD4;
   float3 world_position : TEXCOORD5;
   float3 world_normal : TEXCOORD6;

   float3 precalculated_light : COLOR0;
   float fade : COLOR1;
   float fog_eye_distance : DEPTH;
};

float4 diffuse_blendmap_ps(Ps_blendmap_input input, uniform sampler2D diffuse_maps[3],
                           uniform sampler2D detail_map) : COLOR
{
   float3 diffuse_colors[3];

   for (int i = 0; i < 3; ++i) {
      diffuse_colors[i] = tex2D(diffuse_maps[i], input.texcoords[i]).rgb;
   }

   float3 detail_color = tex2D(detail_map, input.detail_texcoords).rgb;

   float blend_weights[3];

   blend_weights[1] = input.blend_values.b;
   blend_weights[2] = saturate(dot(input.blend_values, float3(0, 1, 0)));
   blend_weights[0] = (1.0 - blend_weights[2]) - blend_weights[1];

   float4 color = 0.0;

   for (i = 0; i < 3; ++i) {
      color.rgb += diffuse_colors[i] * blend_weights[i];
   }
   
   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light);
   light.color = light.color * terrain_constant.x + terrain_constant.y;

   color.rgb *= light.color;

   color.rgb = (color.rgb * detail_color) * 2.0;
   color.a = (input.fade - 0.5) * 4.0;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 diffuse_blendmap_unlit_ps(Ps_blendmap_input input, uniform sampler2D diffuse_maps[3],
                                 uniform sampler2D detail_map) : COLOR
{
   float3 diffuse_colors[3];

   for (int i = 0; i < 3; ++i) {
      diffuse_colors[i] = tex2D(diffuse_maps[i], input.texcoords[i]).rgb;
   }

   float3 detail_color = tex2D(detail_map, input.detail_texcoords).rgb;

   float blend_weights[3];

   blend_weights[1] = input.blend_values.b;
   blend_weights[2] = saturate(dot(input.blend_values, float3(0, 1, 0)));
   blend_weights[0] = (1.0 - blend_weights[2]) - blend_weights[1];

   float4 color = 0.0;

   for (i = 0; i < 3; ++i) {
      color.rgb += diffuse_colors[i] * blend_weights[i];
   }
   
   float3 light_color = hdr_info.zzz * terrain_constant.x + terrain_constant.y;

   color.rgb *= light_color;

   color.rgb = (color.rgb * detail_color) * 2.0;
   color.a = (input.fade - 0.5) * 4.0;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

struct Ps_detail_input
{
   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_map_texcoords : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float3 precalculated_light : COLOR0;
   float3 projection_color : COLOR1;
   float fade : COLOR2;

   float fog_eye_distance : DEPTH;
};

float4 detailing_ps(Ps_detail_input input, uniform sampler2D detail_maps[2],
                    uniform sampler2D projection_map, uniform sampler2D shadow_map) : COLOR
{

   float3 detail_color_0 = tex2D(detail_maps[0], input.detail_texcoords[0]).rgb;
   float3 detail_color_1 = tex2D(detail_maps[1], input.detail_texcoords[1]).rgb;
   float3 projected_color = sample_projected_light(projection_map,
                                                   input.projection_texcoords);
   float shadow_map_color = tex2Dproj(shadow_map, input.shadow_map_texcoords).a;

   // HACK: Ignore projected cube maps.
   if (input.projection_texcoords.z != 0.0) {
      projected_color = 1.0;
   }
   
   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light);

   float3 color;

   float projection_shadow = lerp(0.8, shadow_map_color, 0.8);

   color = projected_color * input.projection_color * projection_shadow;
   color += light.color;

   float shadow = 1 - (input.fade * (1 - shadow_map_color));
   
   float3 blended_detail_color = (detail_color_0 * shadow)  * 2.0;
   blended_detail_color *=  (detail_color_1 * 2.0);
  
   color *= blended_detail_color;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}