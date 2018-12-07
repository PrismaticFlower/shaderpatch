
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float3> far_scene_texture;

const static float4 skyfog_pos_scale = custom_constants[0]; 
const static float4 skyfog_z_transform = custom_constants[1];
const static float4 skyfog_texcoords_transfrom = custom_constants[2]; 

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output skyfog_vs(Vs_input input)
{
   Vs_output output;

   float4 position = float4(input.position, 1.0);

   position *= skyfog_pos_scale;

   output.positionPS.xyw = position.xyz;
   output.positionPS.z = dot(position, skyfog_z_transform);

   output.texcoords = 
      input.texcoords * skyfog_texcoords_transfrom.xy + skyfog_texcoords_transfrom.zw;

   return output;
}

float4 skyfog_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 color = far_scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);

   return float4(color, 1.0);
}