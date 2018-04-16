
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 color : COLOR;
   float4 texcoords : TEXCOORD;
};

struct Vs_output
{
   float4 position : POSITION;
   float4 color : COLOR;
   float2 texcoords : TEXCOORD;
};

sampler diffuse_map;
sampler bump_map;

float4 decal_constants[2] : register(ps, c[0]);

Vs_output decal_vs(Vs_input input)
{
   float4 world_position = transform::position(input.position);

   Vs_output output;

   output.position = position_project(input.position);
   output.texcoords = decompress_texcoords(input.texcoords);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   float4 color;
   color.rgb = input.color.rgb * material_diffuse_color.rgb;
   color.rgb *= hdr_info.rgb;
   color.a = material_diffuse_color.a * near_scene.fade;

   output.color = get_material_color(input.color);

   return output;
}

struct Ps_input
{
   float4 color : COLOR;
   float2 texcoords : TEXCOORD;
};

float4 diffuse_ps(Ps_input input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords);

   float4 color;

   color.rgb = diffuse_color.rgb * input.color.rgb;
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   float alpha_factor = diffuse_color.a * input.color.a;

   color.rgb = lerp(decal_constants[0].rgb, color.rgb, alpha_factor.rrr);
   color.a = decal_constants[0].a;

   return color;
}

float4 diffuse_bump_ps(Ps_input input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.texcoords);
   float4 bump_color = tex2D(bump_map, input.texcoords);

   float4 color;

   color.rgb = dot((bump_color.rgb - 0.5) * 2, (input.color.rgb - 0.5) * 2);
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   color.rgb *= decal_constants[1].rgb;
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   color.a = bump_color.a * input.color.a;

   color.rgb *= diffuse_color.rgb;
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   color.a *= diffuse_color.a;
   color.a = clamp(color.a, 0.0, 1.0);

   color.rgb = lerp(decal_constants[0].rgb, color.rgb, color.aaa);

   return color;
}

float4 bump_ps(Ps_input input) : COLOR
{
   float4 bump_color = tex2D(diffuse_map, input.texcoords);

   float4 color;

   color.rgb = dot((bump_color.rgb - 0.5) * 2, (input.color.rgb - 0.5) * 2);
   color.rgb = clamp(color.rgb, float3(0, 0, 0), float3(1, 1, 1));

   float alpha_factor = clamp(bump_color.a * input.color.a, 0.0, 1.0);

   color.rgb = lerp(decal_constants[0].rgb, color.rgb, alpha_factor.rrr);
   color.a = decal_constants[0].a;

   return color;
}