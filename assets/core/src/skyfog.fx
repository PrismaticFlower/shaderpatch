
#include "constants_list.hlsl"

Texture2D<float3> far_scene_texture;

SamplerState linear_clamp_sampler;

float4 skyfog_pos_scale : register(vs, c[CUSTOM_CONST_MIN]);
float4 skyfog_z_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4 skyfog_texcoords_transfrom : register(vs, c[CUSTOM_CONST_MIN + 2]);

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD;
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

float4 skyfog_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 color = far_scene_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);

   return float4(color, 1.0);
}