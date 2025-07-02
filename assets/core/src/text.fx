
#include "color_utilities.hlsl"
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

// clang-format off

#define ATLAS_GLYPH_COUNT 226

cbuffer FontAtlasIndex : register(MATERIAL_CB_INDEX)
{
   float4 atlas_coords[ATLAS_GLYPH_COUNT];
};

const static float4 interface_scale_offset = custom_constants[0];
const static float4 interface_color = input_color_srgb ? srgb_to_linear(ps_custom_constants[0]) : 
                                                         ps_custom_constants[0];

ByteAddressBuffer glyph_atlas_index : VS_MATERIAL_REGISTER(0);
Texture2D glyph_atlas : PS_MATERIAL_REGISTER(0);

float4 transform_interface_position(float3 position)
{
   float4 positionPS =
      mul(float4(mul(float4(position, 1.0), world_matrix), 1.0), projection_matrix);

   positionPS.xy = positionPS.xy * interface_scale_offset.xy + interface_scale_offset.zw;
   positionPS.xy += vs_pixel_offset * positionPS.w;

   return positionPS;
}

float4 get_interface_color()
{
   float4 color;

   const uint3 friend_color_uint = {1, 86, 213}; // main friend colour
   const uint3 friend_health_color_uint = {1, 76, 187};
   const uint3 foe_color_uint = {223, 32, 32}; // main foe colour
   const uint3 foe_text_color_uint = {150, 30, 30};
   const uint3 foe_text_alt_color_uint = {133, 32, 32};
   const uint3 foe_health_color_uint = {168, 28, 28};
   
   const uint3 interface_color_uint = interface_color.rgb * 255.0;

   if (all(interface_color_uint == friend_color_uint)) {
      color = float4(friend_color, interface_color.a);
   }
   else if (all(interface_color_uint == friend_health_color_uint)) {
      color = float4(friend_health_color, interface_color.a);
   }
   else if (all(interface_color_uint == foe_color_uint)) {
      color = float4(foe_color, interface_color.a);
   }
   else if (all(interface_color_uint == foe_text_color_uint)) {
      color = float4(foe_text_color, interface_color.a);
   }
   else if (all(interface_color_uint == foe_text_alt_color_uint)) {
      color = float4(foe_text_alt_color, interface_color.a);
   }
   else if (all(interface_color_uint == foe_health_color_uint)) {
      color = float4(foe_health_color, interface_color.a);
   }
   else {
      color = interface_color;
   }

   if (input_color_srgb) color = srgb_to_linear(color);

   return color;
}

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float2 atlas_coords : ATLAS;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;

   output.positionPS = transform_interface_position(input.position);

   const int2 texcoords = (int2)input.texcoords;
   const int glyph = texcoords.x >> 2;
   const int2 coords_index = texcoords & 3;

   const float4 glyph_atlas_coords = asfloat(glyph_atlas_index.Load4(glyph * 16));

   output.atlas_coords = float2(glyph_atlas_coords[coords_index.x], glyph_atlas_coords[coords_index.y]);

   return output;
}

float4 main_ps(float2 atlas_coords : ATLAS) : SV_Target0
{
   const float alpha = glyph_atlas.SampleLevel(text_sampler, atlas_coords, 0).a;
   
   return float4(get_interface_color().rgb, interface_color.a * alpha);
}
