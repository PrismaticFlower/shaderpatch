#include "vertex_utilities.hlsl"

float4 sample_scale_offset : register(vs, c[CUSTOM_CONST_MIN]);
float4 sample_positionPS : register(vs, c[CUSTOM_CONST_MIN + 1]);

float4 sample_alpha_scale : register(ps, c[0]);
Texture2D<float4> source : register(ps_3_0, s0);

SamplerState linear_wrap_sampler;

const static float min_value = 1.0 / 3.0;
const static float cutoff = 1.0 / 255.0;

struct Vs_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD;
};

Vs_output sample_vs(int4 sample_locations : POSITION)
{
   Vs_output output;

   float2 locations = decompress_position((float3)sample_locations.xyz).xy;

   output.positionPS = sample_positionPS;
   output.texcoords = (locations.xy + sample_scale_offset.zw) * sample_scale_offset.xy;

   return output;
}

float4 sample_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float alpha = source.SampleLevel(linear_wrap_sampler, texcoords, 0).a;
   alpha  *= sample_alpha_scale.a;

   // Min alpha test for RT formats with only 2 alpha bits.
   return alpha >= cutoff ? max(alpha, min_value) : 0.0;
}