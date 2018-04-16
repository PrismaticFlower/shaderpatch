#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct Vs_output
{
   float4 position : POSITION;
   float4 color : COLOR;
   float fog : FOG;
};

Vs_output lightbeam_vs(Vs_input input)
{
   Vs_output output;

   float4 world_position = transform::position(input.position);

   Near_scene near_scene = calculate_near_scene_fade(world_position);
   near_scene.fade = near_scene.fade * near_scene.fade;

   output.position = position_project(world_position);
   output.fog = calculate_fog(near_scene, world_position);

   near_scene = clamp_near_scene_fade(near_scene);

   float4 material_color = get_material_color(input.color);

   output.color.rgb = material_color.rgb * hdr_info.zzz;
   output.color.a = material_color.a * near_scene.fade;

   return output;
}

float4 lightbeam_ps(float4 color : COLOR) : COLOR
{
   return color;
}