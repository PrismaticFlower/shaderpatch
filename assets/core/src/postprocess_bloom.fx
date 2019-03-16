
#include "pixel_utilities.hlsl"
#include "postprocess_common.hlsl"

Texture2D<float3> bloom_input_texture : register(t0);

float4 downsample_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return float4(bloom_box13_downsample(scene_texture, texcoords), 1.0);
}

float4 upsample_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return float4(bloom_tent9_upsample(scene_texture, texcoords), 1.0);
}

float4 threshold_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const float3 color = bloom_box13_downsample(scene_texture, texcoords);

   if (dot(luma_weights, color) < bloom_threshold) return 0.0;

   return float4(color, 1.0);
}
