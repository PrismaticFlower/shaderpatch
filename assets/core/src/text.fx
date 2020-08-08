
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

// clang-format off

const static float4 interface_scale_offset = custom_constants[0];
const static float4 interface_color = ps_custom_constants[0];

Texture2D<float> glyph_atlas : register(t7);

float4 transform_interface_position(float3 position)
{
   float4 positionPS =
      mul(float4(mul(float4(position, 1.0), world_matrix), 1.0), projection_matrix);

   positionPS.xy = positionPS.xy * interface_scale_offset.xy + interface_scale_offset.zw;
   positionPS.xy += vs_pixel_offset * positionPS.w;

   return positionPS;
}

struct Vs_textured_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_textured_output
{
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_textured_output main_vs(Vs_textured_input input)
{
   Vs_textured_output output;

   output.positionPS = transform_interface_position(input.position);
   output.texcoords = input.texcoords;

   return output;
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const float alpha = glyph_atlas.Sample(linear_clamp_sampler, texcoords);

   return float4(interface_color.rgb, interface_color.a * alpha);

}
