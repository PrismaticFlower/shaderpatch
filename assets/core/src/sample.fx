#include "vertex_utilities.hlsl"


struct Vs_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

float4 sample_scale_offset : register(vs, c[CUSTOM_CONST_MIN]);
float4 sample_position : register(vs, c[CUSTOM_CONST_MIN + 1]);

Vs_output sample_vs(float4 position : POSITION)
{
   Vs_output output;

   position = decompress_position(position);

   output.position = sample_position;
   output.texcoord = (position.xy + sample_scale_offset.zw) * sample_scale_offset.xy;

   return output;
}

uniform float4 sample_alpha_scale : register(ps, c[0]);
sampler2D source;

const static float min_value = 1.0 / 3.0;
const static float cutoff = 1.0 / 255.0;

float4 sample_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float alpha = tex2D(source, texcoord).a;
   alpha  *= sample_alpha_scale.a;

   // Min alpha test for RT formats with only 2 alpha bits.
   return alpha >= cutoff ? max(alpha, min_value) : 0.0;
}