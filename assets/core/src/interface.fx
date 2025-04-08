
#include "color_utilities.hlsl"
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

// clang-format off

const static float4 interface_scale_offset = custom_constants[0];
const static float4 interface_color = input_color_srgb ? srgb_to_linear(ps_custom_constants[0]) : 
                                                         ps_custom_constants[0];

Texture2D<float4> element_texture : register(t0);
Texture2D<float4> element_mask : register(t1);

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
   const uint3 foe_flag_color_uint = {150, 15, 25};
   
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
   else if (all(interface_color_uint == foe_flag_color_uint)) {
      color = float4(foe_flag_color, interface_color.a);
   }
   else {
      color = interface_color;
   }

   if (input_color_srgb) color = srgb_to_linear(color);

   return color;
}

// Masked Bitmap Element

struct Vs_masked_input
{
   float3 position : POSITION;
   float2 texcoords_0 : TEXCOORD0;
   float2 texcoords_1 : TEXCOORD1;
};

struct Vs_masked_output
{
   float2 texcoords[2] : TEXCOORD0;
   float4 positionPS : SV_Position;
};

Vs_masked_output masked_bitmap_vs(Vs_masked_input input)
{
   Vs_masked_output output;
    
   output.positionPS = transform_interface_position(input.position);
   output.texcoords[0] = input.texcoords_0;
   output.texcoords[1] = input.texcoords_1;

   return output;
}

float4 masked_bitmap_ps(float2 texcoords[2] : TEXCOORD0) : SV_Target0
{
   const float4 mask = element_mask.Sample(linear_clamp_sampler, texcoords[1]);
   const float4 texture_color = element_texture.Sample(linear_clamp_sampler, texcoords[0]);

   return texture_color * get_interface_color() * mask;
}

// Vector Element

struct Vs_vector_input
{
   float3 position : POSITION;
   unorm float4 color : COLOR;
};

struct Vs_vector_output
{
   float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Vs_vector_output vector_vs(Vs_vector_input input)
{
   Vs_vector_output output;

   output.positionPS = transform_interface_position(input.position);
   output.color = input.color;

   return output;
}

float4 vector_ps(float4 color : COLOR) : SV_Target0
{
   return color * get_interface_color();
}

// Bitmap Untextured

float4 bitmap_untextured_vs(float3 position : POSITION) : SV_Position
{
   return transform_interface_position(position);
}

float4 bitmap_untextured_ps() : SV_Target0
{
   return get_interface_color();
}

// Bitmap Textured

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

Vs_textured_output bitmap_textured_vs(Vs_textured_input input)
{
   Vs_textured_output output;

   output.positionPS = transform_interface_position(input.position);
   output.texcoords = input.texcoords;

   return output;
}

float4 bitmap_textured_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return element_texture.Sample(linear_clamp_sampler, texcoords) * get_interface_color();

}
