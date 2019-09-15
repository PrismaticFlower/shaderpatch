
#include "scope_blur_common.hlsl"

cbuffer BloomConstants : register(b0)
{
   float threshold;
   float intensity;
}

Texture2D<float4> scene_texture;

SamplerState samp;

const static bool bloom_alt_scope_blur = BLOOM_ALT_SCOPE_BLUR;
const static uint threshold_mip_level = 2;

float4 threshold_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float3 color =
      scene_texture.SampleLevel(samp, texcoords, threshold_mip_level).rgb;

   if (bloom_alt_scope_blur) {
      const float4 scoped =
         scope_blur(texcoords, threshold_mip_level, scene_texture, samp);

      color = lerp(color.rgb, scoped.rgb, scoped.a);
   }

   if (dot(color, float3(0.299, 0.587, 0.114)) < threshold) {
      return float4(0.0, 0.0, 0.0, 1.0);
   }

   return float4(color, 1.0);
}

float4 overlay_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const float3 color = scene_texture.SampleLevel(samp, texcoords, 0).rgb;

   return float4(color * intensity, 1.0);
}

float4 brighten_ps() : SV_Target0
{
   return 1.0;
}
