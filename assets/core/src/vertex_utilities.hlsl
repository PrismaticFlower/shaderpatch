#ifndef VERTEX_UTILS_INCLUDED
#define VERTEX_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "color_utilities.hlsl"

#pragma warning(disable : 3571)

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
   float3 color;

   if (vertex_color_srgb) {
      color = srgb_to_linear(material_diffuse_color.rgb);
   }
   else {
      color = material_diffuse_color.rgb;
   }

   return float4(color, material_diffuse_color.a);
}

float4 get_material_color(float4 color)
{
#ifdef __VERTEX_INPUT_COLOR__
   if (vertex_color_srgb) {
      const float3 vertex_lin = srgb_to_linear(color.rgb * color_state.y + color_state.x);
      const float3 matl_lin = srgb_to_linear(material_diffuse_color.rgb);

      color.rgb = vertex_lin * matl_lin;
   }
   else {
      color.rgb = (color.rgb * color_state.y + color_state.x) * material_diffuse_color.rgb;
   }

   color.a = (color.a * color_state.w + color_state.z) * material_diffuse_color.a;
#else
   return get_material_color();
#endif

   return color;
}

float3 get_static_diffuse_color(float4 color)
{
#ifdef __VERTEX_INPUT_COLOR__   
   if (vertex_color_srgb) {
      return srgb_to_linear(color.rgb * color_state.x + color_state.z);
   }
   else {
      return color.rgb * color_state.x + color_state.z;
   }
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
   tangent = normalize(-normal.x * normal + float3(1.0, 0.0, 0.0));
   bitangent = normalize(-normal.z * normal + float3(0.0, 0.0, 1.0));
}

float calculate_near_fade(float depthPS)
{
   return depthPS * near_fade_scale + near_fade_offset;
}

float calculate_fog(float heightWS, float depthPS)
{
   const float depth_fog = depthPS * depth_fog_scale + depth_fog_offset;
   const float height_fog = heightWS * height_fog_scale + height_fog_offset;

   return saturate(min(depth_fog, height_fog));
}

void calculate_near_fade_and_fog(float3 positionWS, float4 positionPS,
                                 out float near_fade, out float fog)
{
   near_fade = calculate_near_fade(positionPS.z);
   fog = calculate_fog(positionWS.y, positionPS.z);
}

#endif