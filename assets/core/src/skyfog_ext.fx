
#include "vertex_utilities.hlsl"
#include "constants_list.hlsl"

sampler2D far_scene_sampler : register(s0);
sampler2D far_scene_depth_sampler : register(s1);
sampler2D near_scene_depth_sampler : register(s2);

const static float near_scene_near_plane = 0.5;
const static float far_scene_near_plane = 2.0;
const static float far_plane = 10000.0;

float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD) : POSITION
{
   return float4(position, 0.0, 1.0);
}

float get_far_scene_depth(float2 texcoord)
{
   const float depth = tex2D(far_scene_depth_sampler, texcoord).r;
    
   const float proj_a = far_plane / (far_plane - far_scene_near_plane);
   const float proj_b = (-far_plane * far_scene_near_plane) / (far_plane - far_scene_near_plane);

   return (depth < 1e-10) ? far_plane : proj_b / (depth - proj_a);
}

float get_near_scene_depth(float depth)
{
   const float proj_a = far_plane / (far_plane - near_scene_near_plane);
   const float proj_b = (-far_plane * near_scene_near_plane) / (far_plane - near_scene_near_plane);

   return (depth < 1e-10) ? far_plane : proj_b / (depth - proj_a);
}

void main_ps(float2 texcoord : TEXCOORD, out float4 out_color : COLOR0, out float4 out_depth : COLOR1)
{
   float3 far_scene = tex2D(far_scene_sampler, texcoord).rgb;

   float near_scene_raw_depth = tex2D(near_scene_depth_sampler, texcoord).r;
   float near_scene_depth = get_near_scene_depth(near_scene_raw_depth);
   float far_scene_depth = get_far_scene_depth(texcoord);

   float fade = 
      (near_scene_raw_depth < 1e-10) ? 0.0 : saturate(near_scene_depth * near_scene_fade.x + near_scene_fade.y);

   out_depth = float4(min(far_scene_depth, near_scene_depth).xxx, 1.0);
   out_color = float4(far_scene, fade);
}