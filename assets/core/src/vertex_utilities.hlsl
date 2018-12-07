#ifndef VERTEX_UTILS_INCLUDED
#define VERTEX_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"

#pragma warning(disable : 3571)

struct Near_scene
{
   float view_z;
   float fade;
};

float4 get_projection_matrix_row(const uint i)
{
   if (i == 0) {
      return float4(projection_matrix[0].x, projection_matrix[1].x,
         projection_matrix[2].x, projection_matrix[3].x);
   }
   else if (i == 1) {
      return float4(projection_matrix[0].y, projection_matrix[1].y,
         projection_matrix[2].y, projection_matrix[3].y);
   }
   else if (i == 2) {
      return float4(projection_matrix[0].z, projection_matrix[1].z,
         projection_matrix[2].z, projection_matrix[3].z);
   }
   else if (i == 3) {
      return float4(projection_matrix[0].w, projection_matrix[1].w,
         projection_matrix[2].w, projection_matrix[3].w);
   }

   return float4(0, 0, 0, 0);
}

float2 transform_texcoords(float2 texcoords, float4 x_transform, 
                           float4 y_transform)
{
   float2 transformed;

   transformed.x = dot(float4(texcoords, 0.0, 1.0), x_transform);
   transformed.y = dot(float4(texcoords, 0.0, 1.0), y_transform);

   return transformed;
}

float3 decompress_position(float3 position)
{
   position = position_decompress_min.xyz * position;
   return position_decompress_max.xyz + position;
}

float4 get_material_color()
{
   return float4(pow(material_diffuse_color.rgb, vertex_color_gamma), material_diffuse_color.a);
}

float4 get_material_color(float4 color)
{
#ifdef __VERTEX_INPUT_COLOR__
   color.rgb = pow(color.rgb * color_state.y + color_state.x, vertex_color_gamma) *
               pow(material_diffuse_color.rgb, vertex_color_gamma);
   color.a = (color.a * color_state.w + color_state.z) * material_diffuse_color.a;
#else
   return get_material_color();
#endif

   return color;
}


float3 get_static_diffuse_color(float4 color)
{
#ifdef __VERTEX_INPUT_COLOR__
   return pow(color.rgb * color_state.x + color_state.z, vertex_color_gamma);
#else
   return 0.0;
#endif
}

float4 transform_shadowmap_coords(float3 world_position)
{
   float4 coords = 0.0;
   
   coords.xyw = mul(float4(world_position, 1.0), shadow_map_transform).xyz;

   return coords;
}

void generate_terrain_tangents(float3 normal, out float3 tangent, 
                               out float3 bitangent)
{
   //Pandemic's note: we rely on the fact that the object is world axis aligned
   tangent = -normal.x * normal + float3(1.0, 0.0, 0.0);
   tangent *= rsqrt(tangent.x);

   bitangent = -normal.z * normal + float3(0.0, 0.0, 1.0);
   bitangent *= rsqrt(bitangent);
}


Near_scene calculate_near_scene_fade(float3 world_position)
{
   Near_scene result;

   result.view_z = dot(float4(world_position, 1.0), get_projection_matrix_row(3));
   result.fade = result.view_z * near_scene_fade.x + near_scene_fade.y;

   return result;
}

Near_scene clamp_near_scene_fade(Near_scene near_scene)
{
   near_scene.fade = saturate(near_scene.fade);

   return near_scene;
}

float calculate_fog(Near_scene near_scene, float3 world_position)
{
   float x = near_scene.view_z * fog_info.x + fog_info.y;
   float y = world_position.y * fog_info.z + fog_info.w;

   return min(x, y);
}

float calculate_fog(float3 world_position)
{
   float view_z = dot(float4(world_position, 1.0), get_projection_matrix_row(3));

   float x = view_z * fog_info.x + fog_info.y;
   float y = world_position.y * fog_info.z + fog_info.w;

   return min(x, y);
}

#endif