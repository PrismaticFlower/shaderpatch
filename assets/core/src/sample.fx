#include "vertex_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

const static float4 sample_scale_offset = custom_constants[0];
const static float4 sample_positionPS = custom_constants[1];
const static float sample_alpha_scale = ps_custom_constants[0].a;

Texture2D<float4> source : register(t0);

const static float min_value = 1.0 / 3.0;
const static float cutoff = 1.0 / 255.0;

struct Vs_output
{
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output sample_vs(int4 sample_locations : POSITION)
{
   Vs_output output;

   float2 locations = decompress_position((float3)sample_locations.xyz).xy;

   // This should really use the viewport size buuuut we don't have access to it here so the below hack will have to do for now.
   // output.positionPS = sample_positionPS + float4(1.0 / viewport_width, -1.0 / viewport_height, 0.0, 0.0);
   output.positionPS = sample_positionPS + float4(0.01, -0.01, 0.0, 0.0);
   output.texcoords = (locations.xy + sample_scale_offset.zw) * sample_scale_offset.xy;

   return output;
}

float4 sample_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float alpha = source.SampleLevel(linear_clamp_sampler, texcoords, 0).a;
   alpha  *= sample_alpha_scale;

   // Min alpha test for RT formats with only 2 alpha bits.
   return alpha >= cutoff ? max(alpha, min_value) : 0.0;
}