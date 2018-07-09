
#include "ext_constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"

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

   float2 texcoords : TEXCOORD0;
   float3 normal_texcoords : TEXCOORD1;
   float3 world_normal : TEXCOORD2;
   float3 view_normal : TEXCOORD3;
   float fade : TEXCOORD4;
};

float3 animate_normal(float3 normal)
{
   float x_angle = lerp(0, 6.283185, shield_constants.y);
   float y_angle = lerp(0, 6.283185, shield_constants.x);

   float3x3 x_trans = {{cos(x_angle), -sin(x_angle), 0.0},
                       {sin(x_angle), cos(x_angle), 0.0},
                       {0.0, 0.0, 1.0}};

   normal = mul(x_trans, normal);

   float3x3 y_trans = {{cos(y_angle), 0.0, sin(y_angle)},
                       {0.0, 1.0, 0.0},
                       {-sin(y_angle), 0.0, cos(y_angle)}};

   return mul(y_trans, normal);
}

Vs_output shield_vs(Vs_input input)
{
   Vs_output output;

   float3 obj_normal = normalize(-decompress_position(input.position).xyz);
   float3 world_normal = normals_to_world(obj_normal);
   float4 world_position = transform::position(input.position);

   output.world_normal = world_normal;
   output.position = position_project(world_position);

   float3 view_normal = normalize(world_position.xyz - world_view_position);
   output.view_normal = view_normal;

   float3 reflected_view_normal = reflect(view_normal, world_normal);

   float view_angle = dot(reflected_view_normal, reflected_view_normal);
   view_angle = max(view_angle, shield_constants.w);
   
   float angle_alpha_factor = rcp(view_angle);
   view_angle = dot(view_normal, reflected_view_normal);
   angle_alpha_factor *= view_angle;
   angle_alpha_factor = angle_alpha_factor * -0.5 + 0.5;
   angle_alpha_factor *= max(shield_constants.z, 1.0);
   angle_alpha_factor = -angle_alpha_factor * material_diffuse_color.w + material_diffuse_color.w;


   output.texcoords =
      output.texcoords = decompress_texcoords(input.texcoords) + shield_constants.xy;
   output.normal_texcoords = animate_normal(world_normal);

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene.fade = near_scene.fade * near_scene_fade_scale.x + near_scene_fade_scale.y;
   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade = near_scene.fade * angle_alpha_factor;

   output.fade = saturate(near_scene.fade * angle_alpha_factor);
   
   return output;
}

sampler2D diffuse_map : register(ps, s0);
sampler2D normal_map : register(ps, s12);
sampler2D refraction_texture : register(ps, s13);

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float3 normal_texcoords : TEXCOORD1;
   float3 world_normal : TEXCOORD2;
   float3 view_normal : TEXCOORD3;
   float fade : TEXCOORD4;
};

float2 map_xyz_to_uv(float3 pos)
{
   float3 abs_pos = abs(pos);
   float3 pos_positive = sign(pos) * 0.5 + 0.5;

   float max_axis;
   float2 coords;

   if (abs_pos.x >= abs_pos.y && abs_pos.x >= abs_pos.z) {
      max_axis = abs_pos.x;

      coords = lerp(float2(pos.z, pos.y), float2(-pos.z, pos.y), pos_positive.x);
   }
   else if (abs_pos.y >= abs_pos.z) {
      max_axis = abs_pos.y;

      coords = lerp(float2(pos.x, pos.z), float2(pos.x, -pos.z), pos_positive.y);
   }
   else {
      max_axis = abs_pos.z;

      coords = lerp(float2(-pos.x, pos.y), float2(pos.x, pos.y), pos_positive.z);
   }

   return 0.5 * (coords / max_axis + 1.0);
}

float4 shield_ps(Ps_input input, float2 position : VPOS) : COLOR
{
   float3 view_normal = normalize(input.view_normal);
   
   float3 normal = perturb_normal(normal_map, map_xyz_to_uv(input.normal_texcoords),
                                  normalize(input.world_normal), view_normal);

   float2 scene_coords = position * rt_resolution.zw + normal.xz * 0.1;
   float3 scene_color = tex2D(refraction_texture, scene_coords).rgb;
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords);

   const float3 H = normalize(light_directional_0_dir.xyz + view_normal);
   const float NdotH = saturate(dot(normal, H));
   float3 specular = pow(NdotH, 64);
   specular *= specular_color.rgb;

   float3 color = diffuse_color.rgb * material_diffuse_color.rgb;

   float alpha = material_diffuse_color.a * diffuse_color.a;

   color = (color * alpha + specular) * hdr_info.z;

   return float4(color + scene_color, input.fade);
}
