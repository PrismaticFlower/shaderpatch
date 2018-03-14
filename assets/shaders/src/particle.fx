#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_normal_input
{
   float4 position : POSITION;
   float4 color : COLOR;
   float4 texcoords : TEXCOORD0;
};

struct Vs_normal_output
{
   float4 position : POSITION;
   float4 color : COLOR;
   float fog : FOG;
   float2 texcoords : TEXCOORD;
};

Vs_normal_output normal_vs(Vs_normal_input input,
                           uniform float2 fade_factor : register(vs, c[CUSTOM_CONST_MIN]),
                           uniform float4 texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 1]))
{
   Vs_normal_output output;

   float4 world_position = transform::position(input.position);
   float4 position = position_project(world_position);

   output.position = position;

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene = clamp_near_scene_fade(near_scene);
   near_scene.fade *= near_scene.fade;

   output.color.rgb = get_material_color(input.color).rgb * hdr_info.zzz;

   float fade_scale;

   fade_scale = position.w * fade_factor.x + fade_factor.y;
   fade_scale = saturate(fade_scale);

   output.color.a = (near_scene.fade * fade_scale) * get_material_color(input.color).a;

   float2 texcoords = decompress_texcoords(input.texcoords);
   output.texcoords = texcoords * texcoord_transform.xy + texcoord_transform.zw;

   return output;
}

struct Vs_blur_input
{
   float4 position : POSITION;
   float4 color : COLOR;
   float4 texcoords : TEXCOORD0;
   float3 normal : NORMAL;
};

struct Vs_blur_output
{
   float4 position : POSITION;
   float4 color : COLOR;
   float  fog : FOG;
   float2 texcoords : TEXCOORD0;
   float4 blur_texcoords : TEXCOORD1;
};

Vs_blur_output blur_vs(Vs_blur_input input,
                       uniform float2 fade_factor : register(vs, c[CUSTOM_CONST_MIN]),
                       uniform float4 texcoord_transform : register(vs, c[CUSTOM_CONST_MIN + 1]),
                       uniform float4x4 blur_projection : register(vs, c[CUSTOM_CONST_MIN + 2]))
{
   Vs_blur_output output;

   float4 world_position = transform::position(input.position);
   float4 position = position_project(world_position);

   output.position = position;

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene = clamp_near_scene_fade(near_scene);

   output.color.rgb = get_material_color(input.color).rgb;

   float fade_scale;

   fade_scale = position.w * fade_factor.x + fade_factor.y;
   fade_scale = saturate(fade_scale);

   output.color.a = (near_scene.fade * fade_scale) * get_material_color(input.color).a;

   float2 texcoords = decompress_texcoords(input.texcoords);
   output.texcoords = texcoords * texcoord_transform.xy + texcoord_transform.zw;

   float3 normal = decompress_normals(input.normal);

   float4 coords = float4(normal.xyz, 1.0);
   
   output.blur_texcoords = mul(coords, blur_projection);

   return output;
}

struct Ps_normal_input
{
   float4 color : COLOR;
   float2 texcoords : TEXCOORD;
};

float4 normal_ps(Ps_normal_input input,
                 uniform sampler2D diffuse_map) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords);

   return diffuse_color * input.color;
}

struct Ps_blur_input
{
   float4 color : COLOR;
   float2 texcoords : TEXCOORD0;
   float4 blur_texcoords : TEXCOORD1;
};

float4 blur_ps(Ps_blur_input input,
               uniform sampler2D alpha_map,
               uniform sampler2D blur_buffer) : COLOR
{
   float alpha = tex2D(alpha_map, input.texcoords).a;
   float4 refraction_color = tex2Dproj(blur_buffer, input.blur_texcoords);
   
   float4 color;

   color.rgb = refraction_color.rgb * input.color.rgb;
   color.a = alpha * input.color.a;

   return color;
}