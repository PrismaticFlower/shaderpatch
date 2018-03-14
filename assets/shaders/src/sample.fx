#include "vertex_utilities.hlsl"


struct Vs_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

sampler texture_sampler;

float4 sample_const_0 : register(vs, c[CUSTOM_CONST_MIN]);
float4 sample_const_1 : register(vs, c[CUSTOM_CONST_MIN + 1]);

Vs_output sample_vs(float4 position : POSITION)
{
   Vs_output output;

   position = decompress_position(position);

   output.position = sample_const_1;
   output.texcoord = (position.xy + sample_const_0.zw) * sample_const_0.xy;

   return output;
}

float4 sample_ps(float2 texcoord : TEXCOORD,
                 uniform float4 constant : register(ps, c[0])) : COLOR
{
   float4 color = tex2D(texture_sampler, texcoord);

   return color.aaaa * constant;
}