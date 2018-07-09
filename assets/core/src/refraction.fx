
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"

uniform float4 distort_constant : register(vs, c[CUSTOM_CONST_MIN]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normal : NORMAL;
   float4 weights : BLENDWEIGHT;
   uint4 blend_indices : BLENDINDICES;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_nodistortion_output
{
   float4 position : POSITION;
   float2 texcoords : TEXCOORD;
   float3 world_position : TEXCOORD1;
   float3 world_normal : TEXCOORD2;

   float4 color : TEXCOORD3;
   float3 precalculated_light : TEXCOORD4;

   float fog_eye_distance : DEPTH;
};

Vs_nodistortion_output far_vs(Vs_input input,
                              uniform float4 texcoord_transform[2] : register(vs, c[CUSTOM_CONST_MIN]))
{
   Vs_nodistortion_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 world_normal = normals_to_world(transform::normals(input.normal, input.blend_indices,
                                          input.weights));

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);
   output.texcoords = decompress_transform_texcoords(input.texcoords, texcoord_transform[0],
                                                     texcoord_transform[1]);

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   output.precalculated_light = lighting.diffuse.rgb;
   output.color = get_material_color(input.color);

   return output;
}

Vs_nodistortion_output nodistortion_vs(Vs_input input,
                                       uniform float4 texcoord_transform[2] : register(vs, c[CUSTOM_CONST_MIN]))
{
   Vs_nodistortion_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 world_normal = normals_to_world(transform::normals(input.normal, input.blend_indices,
                                          input.weights));

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);
   output.texcoords = decompress_transform_texcoords(input.texcoords, texcoord_transform[0],
                                                     texcoord_transform[1]);
   
   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   output.precalculated_light = lighting.diffuse.rgb;
   output.color = get_material_color(input.color);

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade *= near_scene.fade;

   output.color.a = saturate(output.color.a * near_scene.fade);

   return output;
}

struct Vs_distortion_output
{
   float4 position : POSITION;

   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 color : TEXCOORD6;
   float3 precalculated_light : TEXCOORD7;
   float3 projection_color : TEXCOORD8;

   float fog_eye_distance : DEPTH;
};

float4 distortion_texcoords(float3 normal, float4 position, float4x4 distort_transform)
{
   float4 distortion = normal.xyzz * distort_constant.zzzw + position;

   return mul(distortion, distort_transform).xyww;
}

Vs_distortion_output distortion_vs(Vs_input input,
                                   uniform float4x4 distort_transform : register(vs, c[CUSTOM_CONST_MIN + 1]),
                                   uniform float4 texcoord_transform[4] : register(vs, c[CUSTOM_CONST_MIN + 5]))
{
   Vs_distortion_output output;

   float4 position = transform::position_obj(input.position, input.blend_indices,
                                             input.weights);
   float4 world_position = position_to_world(position);
   float3 normal = transform::normals(input.normal, input.blend_indices, input.weights);
   float3 world_normal = normals_to_world(normal);

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.distortion_texcoords = distortion_texcoords(normal, position, distort_transform);
   output.projection_texcoords = mul(world_position, light_proj_matrix).xyww;

   output.bump_texcoords = decompress_transform_texcoords(input.texcoords,
                                                          texcoord_transform[0],
                                                          texcoord_transform[1]);
   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texcoord_transform[2],
                                                             texcoord_transform[3]);

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   float4 material_color = get_material_color(input.color);

   float3 projection_color;
   projection_color = lighting.static_diffuse.a * material_color.rgb;
   projection_color *= hdr_info.z;
   projection_color *= light_proj_color.rgb;
   output.projection_color = projection_color;

   output.precalculated_light = lighting.diffuse.rgb;
   output.color.rgb = material_color.rgb;

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade *= near_scene.fade;

   output.color.a = saturate(material_color.a * near_scene.fade);

   return output;
}

struct Ps_far_input
{
   float2 texcoords : TEXCOORD;
   float3 world_position : TEXCOORD1;
   float3 world_normal : TEXCOORD2;

   float4 color : TEXCOORD3;
   float3 precalculated_light : TEXCOORD4;

   float fog_eye_distance : DEPTH;
};

float4 far_ps(Ps_far_input input,
              uniform sampler2D diffuse_map : register(ps, s0)) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords) * input.color;
   
   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);

   float3 color = light.color * diffuse_color.rgb;
   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, diffuse_color.a);
}

float4 distort_alpha_constant : register(ps, c[0]);
const static float distort_blend = distort_alpha_constant.a;

sampler2D bump_map : register(ps, s0);
sampler2D refraction_map : register(ps, s13);
sampler2D diffuse_map : register(ps, s2);
sampler2D projection_map : register(ps, s3);

struct Ps_near_input
{
   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 color : TEXCOORD6;
   float3 precalculated_light : TEXCOORD7;
   float3 projection_color : TEXCOORD8;

   float fog_eye_distance : DEPTH;
};

float4 near_diffuse_ps(Ps_near_input input) : COLOR
{
   float3 distortion_color = tex2Dproj(refraction_map, input.distortion_texcoords).rgb;
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float3 projection_color = tex2Dproj(projection_map, input.projection_texcoords).rgb;

   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);

   float3 color = (diffuse_color.rgb * input.color.rgb) * light.color;
   color += (projection_color * input.projection_color);

   float blend = -diffuse_color.a * distort_blend + diffuse_color.a;
   color = lerp(distortion_color, color, blend);

   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, input.color.a);
}

float4 near_ps(Ps_near_input input) : COLOR
{
   float3 distortion_color = tex2Dproj(refraction_map, input.distortion_texcoords).rgb;
   float3 projection_color = tex2Dproj(projection_map, input.projection_texcoords).rgb;

   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);

   float3 color = input.color.rgb * light.color;
   color += (projection_color * input.projection_color);

   color = lerp(distortion_color, color, distort_blend);

   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, input.color.a);
}

float3 sample_distort_scene(float4 distort_texcoords, float2 bump)
{
   const static float2x2 bump_transform = {0.002f, -0.015f, 0.015f, 0.002f};

   float2 texcoords = distort_texcoords.xy / distort_texcoords.w;
   texcoords += mul(bump, bump_transform);

   return tex2D(refraction_map, texcoords).rgb;
}

float4 near_bump_ps(Ps_near_input input) : COLOR
{
   float2 bump = tex2D(bump_map, input.bump_texcoords).xy;
   float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump);
   float3 projection_color = tex2Dproj(projection_map, input.projection_texcoords).rgb;

   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);

   float3 color =  input.color.rgb * light.color;
   color += (projection_color * input.projection_color);

   color = lerp(distortion_color, color, distort_blend);

   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, input.color.a);
}

float4 near_diffuse_bump_ps(Ps_near_input input) : COLOR
{
   float2 bump = tex2D(bump_map, input.bump_texcoords).xy;
   float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump);
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float3 projection_color = tex2Dproj(projection_map, input.projection_texcoords).rgb;

   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);

   float3 color = (diffuse_color.rgb * input.color.rgb) * light.color;
   color += (projection_color * input.projection_color);

   float blend = -diffuse_color.a * distort_blend + diffuse_color.a;
   color = lerp(distortion_color, color, blend);

   color = fog::apply(color.rgb, input.fog_eye_distance);

   return float4(color, input.color.a);
}
