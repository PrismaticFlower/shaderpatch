
#include "fullscreen_tri_vs.hlsl"

Texture2D<float3> source_texture : register(t0);

float2 blur_dir_size : register(c60);

float4 blur_4_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[3] = {0.273438, 0.328125, 0.035156};
   const static float offsets[3] = {0.000000, 1.333333, 3.111111};

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 3; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

float4 blur_6_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[5] = {0.196381, 0.296753, 0.094421, 0.010376, 0.000259};
   const static float offsets[5] = {0.000000, 1.411765, 3.294118, 5.176471, 7.058824};

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 5; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

float4 blur_8_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[4] = {0.225585937500, 0.314208984375, 0.069824218750, 0.003173828125, };
   const static float offsets[4] = {0.000000000000, 1.384615384615, 3.230769230769, 5.076923076923, };

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 4; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

float4 blur_12_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[7] = {0.1611803, 0.2656817, 0.1217708, 0.0286520, 0.0031668, 0.0001371, 0.0000015};
   const static float offsets[7] = {0.0000000, 1.4400000, 3.3600000, 5.2800000, 7.2000000, 9.1200000, 11.0400000};

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 4; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

float4 blur_16_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[9] = {0.13994993, 0.24148224, 0.13345071, 0.04506128, 0.00897960, 0.00099466, 0.00005526, 0.00000127, 0.00000001};
   const static float offsets[9] = {0.00000000, 1.45454545, 3.39393939, 5.33333333, 7.27272727, 9.21212121, 11.15151515, 13.09090909, 15.03030303};

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 4; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

float4 blur_20_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const static float weights[11] = {0.125370687620, 0.222519402269, 0.137865281840, 0.057691317939, 0.016025366094, 0.002873513920, 0.000318635616, 0.000020447205, 0.000000681574, 0.000000009695, 0.000000000037};
   const static float offsets[11] = {0.000000000000, 1.463414634146, 3.414634146341, 5.365853658537, 7.317073170732, 9.268292682927, 11.219512195122, 13.170731707317, 15.121951219512, 17.073170731707, 19.024390243902};

   float3 color = source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0) * weights[0];

   [unroll] for (int i = 1; i < 4; ++i) {
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords + (offsets[i] * blur_dir_size), 0) * weights[i];
      color += source_texture.SampleLevel(linear_clamp_sampler, texcoords - (offsets[i] * blur_dir_size), 0) * weights[i];
   }

   return float4(color, 1.0);
}

