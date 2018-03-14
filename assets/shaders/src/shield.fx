
#include "ext_constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "transform_utilities.hlsl"

float3 specular_color : register(c[CUSTOM_CONST_MIN]);
float4 shield_constants : register(c[CUSTOM_CONST_MIN + 3]);
float2 near_scene_fade_scale : register(c[CUSTOM_CONST_MIN + 4]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   float4 texcoords : TEXCOORD0;
};

struct Vs_output
{
   float4 position : POSITION;
   float fog : FOG;

   float4 color : COLOR0;
   float fade : COLOR1;

   float2 texcoords : TEXCOORD0;
   float3 world_normal : TEXCOORD1;
   float3 view_normal : TEXCOORD2;
};

Vs_output shield_vs(Vs_input input)
{
   Vs_output output;

   float3 world_normal = normals_to_world(decompress_normals(input.normals));
   float4 world_position = transform::position(input.position);

   output.world_normal = world_normal;
   output.position = transform::position_project(input.position);

   float3 view_normal = world_position.xyz - world_view_position.xyz;
   float view_distance = length(view_normal);
   view_normal = normalize(view_normal);

   float3 reflected_view_normal = reflect(view_normal, world_normal);

   float view_angle = dot(reflected_view_normal, reflected_view_normal);
   view_angle = max(view_angle, shield_constants.w);
   
   float angle_alpha_factor = rcp(view_angle);
   view_angle = dot(view_normal, reflected_view_normal);
   angle_alpha_factor *= view_angle;
   angle_alpha_factor = angle_alpha_factor * -0.5 + 0.5;
   angle_alpha_factor *= max(shield_constants.z, 1.0);
   angle_alpha_factor = -angle_alpha_factor * material_diffuse_color.w + material_diffuse_color.w;

   output.view_normal = view_normal;
   output.texcoords = decompress_texcoords(input.texcoords) + shield_constants.xy;

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene.fade = near_scene.fade * near_scene_fade_scale.x + near_scene_fade_scale.y;
   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade = near_scene.fade * angle_alpha_factor;

   output.fade = near_scene.fade * angle_alpha_factor;

   output.color.rgb = material_diffuse_color.rgb;
   output.color.a = material_diffuse_color.a;
   
   return output;
}

sampler2D diffuse_map : register(ps, s0);
sampler2D normal_map : register(ps, s8);
sampler2D refraction_texture : register(ps, s9);

struct Ps_input
{
   float4 color : COLOR0;
   float fade : COLOR1;

   float2 texcoords : TEXCOORD0;
   float3 world_normal : TEXCOORD1;
   float3 view_normal : TEXCOORD2;
};

float4 shield_ps(Ps_input input, float2 position : VPOS) : COLOR
{
   float3 view_normal = normalize(input.view_normal);
   float3 world_normal = normalize(input.world_normal);

   float3 normal = perturb_normal(normal_map, input.texcoords, world_normal, view_normal);

   float2 scene_coords = position * rt_resolution.zw + normal.xy * 0.1;
   float3 scene_color = tex2D(refraction_texture, scene_coords).rgb;
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords);

   float3 half_vector = normalize(view_normal - light_directional_0_dir.xyz);

   float specular_angle = max(dot(half_vector, normal), 0.0);
   float3 specular = pow(specular_angle, 64);

   specular = specular * specular_color;

   float alpha_value = diffuse_color.a + 0.49;

   if (alpha_value <= 0.5) specular = 0.0;

   float3 color = material_diffuse_color.rgb;

   float alpha = material_diffuse_color.a * diffuse_color.a;

   color = (color * alpha + specular) * hdr_info.z;

   return float4(color + scene_color, input.fade);
}
