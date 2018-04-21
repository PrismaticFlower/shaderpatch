
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_nodistortion_output
{
   float4 position : POSITION;
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
   float1 fog : FOG;
};

Vs_nodistortion_output far_vs(Vs_input input,
                              uniform float4 texcoord_transform[2] : register(vs, c[CUSTOM_CONST_MIN]))
{
   Vs_nodistortion_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.position = position_project(world_position);
   output.texcoords = decompress_transform_texcoords(input.texcoords, texcoord_transform[0],
                                                     texcoord_transform[1]);

   float3 normals = transform::normals(input.normals, input.blend_indices,
                                       input.weights);

   Lighting lighting = light::calculate(normals, world_position.xyz,
                                        get_static_diffuse_color(input.color));

   output.color = get_material_color(input.color);
   output.color.rgb *= lighting.diffuse.rgb;

   output.fog = calculate_fog(world_position);

   return output;
}

Vs_nodistortion_output nodistortion_vs(Vs_input input,
                                       uniform float4 texcoord_transform[2] : register(vs, c[CUSTOM_CONST_MIN]))
{
   Vs_nodistortion_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.position = position_project(world_position);
   output.texcoords = decompress_transform_texcoords(input.texcoords, texcoord_transform[0],
                                                     texcoord_transform[1]);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   output.fog = calculate_fog(near_scene, world_position);

   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade *= near_scene.fade;

   float3 normals = transform::normals(input.normals, input.blend_indices,
                                       input.weights);

   Lighting lighting = light::calculate(normals, world_position.xyz,
                                        get_static_diffuse_color(input.color));

   output.color = get_material_color(input.color);
   output.color.rgb *= lighting.diffuse.rgb;
   output.color.a *= near_scene.fade;

   return output;
}

struct Vs_distortion_output
{
   float4 position : POSITION;
   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;
   float4 color_0 : COLOR0;
   float3 color_1 : COLOR1;
   float1 fog : FOG;
};

Vs_distortion_output distortion_vs(Vs_input input,
                                   uniform float4 distort_constant : register(vs, c[CUSTOM_CONST_MIN]),
                                   uniform float4 texcoord_transform[8] : register(vs, c[CUSTOM_CONST_MIN + 1]))
{
   Vs_distortion_output output;

   float4 position = transform::position_obj(input.position, input.blend_indices,
                                             input.weights);
   float4 world_position = position_to_world(position);

   output.position = position_project(world_position);

   float3 normals = transform::normals(input.normals, input.blend_indices,
                                       input.weights);

   float4 distortion = normals.xyzz * distort_constant.zzzw + position;
   output.distortion_texcoords.x = dot(distortion, texcoord_transform[0]);
   output.distortion_texcoords.y = dot(distortion, texcoord_transform[1]);
   output.distortion_texcoords.zw = dot(distortion, texcoord_transform[3]);

   output.projection_texcoords = mul(world_position, light_proj_matrix);
   output.projection_texcoords.z = output.projection_texcoords.w;

   Lighting lighting = light::calculate(normals, world_position.xyz,
                                        get_static_diffuse_color(input.color));

   float4 material_color = get_material_color(input.color);
   float3 projection_color;

   projection_color = material_color.rgb * lighting.static_diffuse.a;
   projection_color *= hdr_info.zzz;
   output.color_1 = projection_color * light_proj_color.rgb;

   output.bump_texcoords = decompress_transform_texcoords(input.texcoords,
                                                          texcoord_transform[4],
                                                          texcoord_transform[5]);

   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texcoord_transform[6],
                                                             texcoord_transform[7]);

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade *= near_scene.fade;

   output.color_0.rgb = material_color.rgb * lighting.diffuse.rgb;
   output.color_0.a = material_color.a * near_scene.fade;

   return output;
}

struct Ps_far_input
{
   float2 texcoords : TEXCOORD;
   float4 color : COLOR;
};

float4 far_ps(Ps_far_input input,
              uniform sampler diffuse_map) : COLOR
{
   return tex2D(diffuse_map, input.texcoords) * input.color;
}

float4 distort_alpha_constant : register(ps, c[0]);

sampler bump_map : register(ps, s[0]);
sampler distortion_map : register(ps, s[1]);
sampler diffuse_map : register(ps, s[2]);
sampler projection_map : register(ps, s[3]);

struct Ps_near_input
{
   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;
   float4 color_0 : COLOR0;
   float3 color_1 : COLOR1;
};

float4 near_diffuse_ps(Ps_near_input input) : COLOR
{
   float4 distortion_color = tex2Dproj(distortion_map, input.distortion_texcoords);
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 projection_color = tex2Dproj(projection_map, input.projection_texcoords);

   float3 color;

   color = projection_color.rgb * input.color_1.rgb + input.color_0.rgb;
   color *= diffuse_color.rgb;

   float alpha = -diffuse_color.a * distort_alpha_constant.a + diffuse_color.a;
   color = lerp(distortion_color.rgb, color, alpha);

   return float4(color, input.color_0.a);
}

float4 near_ps(Ps_near_input input) : COLOR
{
   float4 distortion_color = tex2Dproj(distortion_map, input.distortion_texcoords);
   float4 projection_color = tex2Dproj(projection_map, input.projection_texcoords);

   float3 color;

   color = projection_color.rgb * input.color_1.rgb + input.color_0.rgb;

   color = lerp(distortion_color.rgb, color, distort_alpha_constant.a);

   return float4(color, input.color_0.a);
}

float4 near_bump_ps(Ps_near_input input) : COLOR
{
   float4 projection_color = tex2Dproj(projection_map, input.projection_texcoords);

   float2 bump_offset = tex2D(bump_map, input.bump_texcoords).xy;
   bump_offset = (bump_offset * 2.0) - 1.0;

   float4 distortion_texcoords = input.distortion_texcoords;
   distortion_texcoords.xy += (bump_offset.xy * 0.1);

   float4 distortion_color = tex2Dproj(distortion_map, distortion_texcoords);

   float3 color = projection_color.rgb * input.color_1.rgb + input.color_0.rgb;

   color = lerp(distortion_color.rgb, color, distort_alpha_constant.a);

   return float4(color, input.color_0.a);
}

float4 near_diffuse_bump_ps(Ps_near_input input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 projection_color = tex2Dproj(projection_map, input.projection_texcoords);

   float2 bump_offset = tex2D(bump_map, input.bump_texcoords).xy;
   bump_offset = (bump_offset * 2.0) - 1.0;

   float4 distortion_texcoords = input.distortion_texcoords;
   distortion_texcoords.xy += (bump_offset.xy * 0.1);

   float4 distortion_color = tex2Dproj(distortion_map, distortion_texcoords);

   float3 color;

   color = projection_color.rgb * input.color_1.rgb + input.color_0.rgb;
   color *= diffuse_color.rgb;

   float alpha = -diffuse_color.a * distort_alpha_constant.a + diffuse_color.a;
   color = lerp(distortion_color.rgb, color, alpha);

   return float4(color, input.color_0.a);
}
