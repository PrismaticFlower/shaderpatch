
#include "fullscreen_tri_vs.hlsl"
#include "pixel_utilities.hlsl"
#include "postprocess_common.hlsl"


float4 downsample_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(bloom_box13_downsample(scene_sampler, texcoords), 1.0);
}

float4 upsample_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(bloom_tent9_upsample(scene_sampler, texcoords), 1.0);
}

float4 threshold_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 color = bloom_box13_downsample(scene_sampler, texcoords);

   if (dot(stock_hdr ? fxaa_luma_weights : luma_weights, color) < bloom_threshold) return 0.0;

   return float4(color, 1.0);
}
