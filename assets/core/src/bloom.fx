
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD,
               uniform float2 pixel_offset : register(c110)) : POSITION
{
   return float4(position + pixel_offset, 0.0, 1.0);
}

float4 source_metrics : register(c60);
float threshold : register(c61);
float3 global_scale : register(c62);
float3 local_scales[5] : register(c63);
float3 dirt_scale : register(c68);

const static float3 luma_weights = {0.2126, 0.7152, 0.0722};

sampler2D source : register(s0);
sampler2D combine_sources[4] : register(s1);
sampler2D dirt_sampler : register(s5);

float4 threshold_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 color = tex2D(source, texcoords).rgb;

   if (dot(luma_weights, color) < threshold) return 0.0;

   return float4(color, 1.0);
}

float4 blur_5_ps(float2 texcoords : TEXCOORD) : COLOR
{
   const static float centre_weight = 0.083355;
   const static float mid_weight = 0.10267899999999999;
   const static float mid_offset = 1.3446761265692109;
   const static float corner_weight = 0.126482;
   const static float2 corner_offset = {1.3446735503866163, 1.3446735503866163};

   float3 color = tex2D(source, texcoords).rgb * centre_weight;

   color += tex2D(source, texcoords + (mid_offset * source_metrics.xy) * float2(1,0)).rgb * mid_weight;
   color += tex2D(source, texcoords - (mid_offset * source_metrics.xy) * float2(1,0)).rgb * mid_weight;
   color += tex2D(source, texcoords + (mid_offset * source_metrics.xy) * float2(0,1)).rgb * mid_weight;
   color += tex2D(source, texcoords - (mid_offset * source_metrics.xy) * float2(0,1)).rgb * mid_weight;

   color += tex2D(source, texcoords + (corner_offset * source_metrics.xy)).rgb * corner_weight;
   color += tex2D(source, texcoords + (corner_offset * source_metrics.xy) * float2(-1, 1)).rgb * corner_weight;
   color += tex2D(source, texcoords + (corner_offset * source_metrics.xy) * float2(-1, -1)).rgb * corner_weight;
   color += tex2D(source, texcoords + (corner_offset * source_metrics.xy) * float2(1, -1)).rgb * corner_weight;

   return float4(color, 1.0);
}

float3 blur_9(float2 texcoords, float2 dir)
{
   // Blur Width 9
   const static float weights[3] = {0.273438, 0.328125, 0.035156};
   const static float offsets[3] = {0.000000, 1.333333, 3.111111};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   [unroll] for (int i = 1; i < 3; ++i) {
      color += 
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 blur_9_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_9(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_9_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_9(texcoords, float2(0.0, 1.0)), 1.0);
}

float3 blur_17(float2 texcoords, float2 dir)
{
   const static float weights[5] = {0.196381, 0.296753, 0.094421, 0.010376, 0.000259};
   const static float offsets[5] = {0.000000, 1.411765, 3.294118, 5.176471, 7.058824};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   [unroll] for (int i = 1; i < 5; ++i) {
      color +=
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 blur_17_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_17(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_17_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_17(texcoords, float2(0.0, 1.0)), 1.0);
}

float3 blur_25(float2 texcoords, float2 dir)
{
   const static float weights[7] = {0.1611803, 0.2656817, 0.1217708, 0.0286520, 0.0031668, 0.0001371, 0.0000015};
   const static float offsets[7] = {0.0000000, 1.4400000, 3.3600000, 5.2800000, 7.2000000, 9.1200000, 11.0400000};
   
   float3 color = tex2D(source, texcoords).rgb * weights[0];

   [unroll] for (int i = 1; i < 7; ++i) {
      color +=
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 blur_25_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_25(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_25_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_25(texcoords, float2(0.0, 1.0)), 1.0);
}

float3 blur_33(float2 texcoords, float2 dir)
{
   const static float weights[9] = {0.13994993, 0.24148224, 0.13345071, 0.04506128, 0.00897960, 0.00099466, 0.00005526, 0.00000127, 0.00000001};
   const static float offsets[9] = {0.00000000, 1.45454545, 3.39393939, 5.33333333, 7.27272727, 9.21212121, 11.15151515, 13.09090909, 15.03030303};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   [unroll] for (int i = 1; i < 9; ++i) {
      color +=
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 blur_33_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_33(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_33_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_33(texcoords, float2(0.0, 1.0)), 1.0);
}

float3 blur_41(float2 texcoords, float2 dir)
{
   const static float weights[11] = {0.125370687620, 0.222519402269, 0.137865281840, 0.057691317939, 0.016025366094, 0.002873513920, 0.000318635616, 0.000020447205, 0.000000681574, 0.000000009695, 0.000000000037};
   const static float offsets[11] = {0.000000000000, 1.463414634146, 3.414634146341, 5.365853658537, 7.317073170732, 9.268292682927, 11.219512195122, 13.170731707317, 15.121951219512, 17.073170731707, 19.024390243902};
   
   float3 color = tex2D(source, texcoords).rgb * weights[0];

   [unroll] for (int i = 1; i < 11; ++i) {
      color +=
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 blur_41_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_33(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_41_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_33(texcoords, float2(0.0, 1.0)), 1.0);
}

float4 combine_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 result = 0.0;

   result += tex2D(source, texcoords).rgb * local_scales[0];
   result += tex2D(combine_sources[0], texcoords).rgb * local_scales[1];
   result += tex2D(combine_sources[1], texcoords).rgb * local_scales[2];
   result += tex2D(combine_sources[2], texcoords).rgb * local_scales[3];
   result += tex2D(combine_sources[3], texcoords).rgb * local_scales[4];

   return float4(result * global_scale, 1.0f);
}

float4 dirt_combine_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float4 result = combine_ps(texcoords);

   result.rgb += 
      result.rgb * (tex2D(dirt_sampler, texcoords).rgb * dirt_scale.rgb);

   return result;
}

