
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

ByteAddressBuffer glyph_atlas_index : register(t1);
Texture2D glyph_atlas : register(t7);

float4 transform_interface_position(float3 position)
{
   float4 positionPS =
      mul(float4(mul(float4(position, 1.0), world_matrix), 1.0), projection_matrix);

   positionPS.xy = positionPS.xy * interface_scale_offset.xy + interface_scale_offset.zw;
   positionPS.xy += vs_pixel_offset * positionPS.w;

   return positionPS;
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
   
   return float4(interface_color.rgb, interface_color.a * alpha);
}
