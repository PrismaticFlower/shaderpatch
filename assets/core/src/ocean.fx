
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   float4 texcoords : TEXCOORD0;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 position : POSITION;
   float2 base_texcoords : TEXCOORD0;
   float2 foam_texcoords : TEXCOORD1;
   float3 normal_texcoords : TEXCOORD2;
   float4 color : COLOR;
   float1 fog : FOG;
};

float4 texcoord_transform[2] : register(vs, c[CUSTOM_CONST_MIN]);

Vs_output near_vs(Vs_input input)
{
   Vs_output output;

   float2 texcoords = decompress_transform_texcoords(input.texcoords,
      texcoord_transform[0],
      texcoord_transform[1]);

   output.base_texcoords = texcoords;
   output.foam_texcoords = texcoords;
   output.normal_texcoords = normals_to_world(decompress_normals(input.normals));

   float4 world_position = transform::position(input.position);

   output.position = position_project(world_position);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   output.fog = calculate_fog(near_scene, world_position);

   output.color.rgb = get_material_color(input.color).rgb;
   output.color.a = near_scene.fade * 0.25 + 0.5;

   return output;
}

Vs_output far_vs(Vs_input input)
{
   Vs_output output = near_vs(input);

   output.color = get_material_color(input.color);

   return output;
}

struct Ps_input
{
   float2 base_texcoords : TEXCOORD0;
   float2 foam_texcoords : TEXCOORD1;
   float3 normal_texcoords : TEXCOORD2;
   float4 color : COLOR;
};

float4 light_vector : register(ps, c[0]);
float4 light_color : register(ps, c[1]);
float4 ambient_color : register(ps, c[2]);

sampler2D base_sampler;
sampler2D foam_sampler;
samplerCUBE normalization_sampler;

float4 near_ps(Ps_input input) : COLOR
{
   float4 base_color = tex2D(base_sampler, input.base_texcoords);
   float4 foam_color = tex2D(foam_sampler, input.base_texcoords);
   float4 normal_color = texCUBE(normalization_sampler, input.normal_texcoords);
   
   float3 normal_light = clamp(dot(normal_color.rgb * 2.0, light_vector.rgb), 0.0, 1.0);
   float foam_level = foam_color.a * input.color.a;

   float4 color = base_color * input.color;

   normal_light = normal_light * light_color.rgb + ambient_color.rgb;

   color.rgb *= normal_light.rgb;

   color.rgb = lerp(color.rgb, foam_color.rgb, foam_level.rrr);

   color.a = (input.color.a - 0.5) * 4.0;

   return color;
}

float4 far_ps(Ps_input input) : COLOR
{
   float4 color = near_ps(input);

   color.a = tex2D(foam_sampler, input.base_texcoords).a * input.color.a;

   return color;
}