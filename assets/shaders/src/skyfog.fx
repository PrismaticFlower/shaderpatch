
#include "vertex_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 texcoord : TEXCOORD;
};

struct Vs_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

sampler sky_sampler;

float4 skyfog_custom_0 : register(vs, c[CUSTOM_CONST_MIN]);
float4 skyfog_custom_1 : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4 skyfog_custom_2 : register(vs, c[CUSTOM_CONST_MIN + 2]);

Vs_output skyfog_vs(Vs_input input)
{
   Vs_output output;

   float4 position = decompress_position(input.position);

   position *= skyfog_custom_0;

   output.position.xyw = position.xyz;
   output.position.z = dot(position, skyfog_custom_1);

   output.texcoord.xy = input.texcoord.xy * skyfog_custom_2.xy + skyfog_custom_2.zw;

   return output;
}

float4 skyfog_ps(float2 texcoord : TEXCOORD) : COLOR
{
   return float4(tex2D(sky_sampler, texcoord).rgb, 1.0);
}