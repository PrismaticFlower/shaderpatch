#ifndef POSTPROCESS_COMMON_INCLUDED
#define POSTPROCESS_COMMON_INCLUDED

sampler2D scene_sampler : register(s0);
sampler2D bloom_sampler : register(s1);
sampler2D dirt_sampler : register(s2);
sampler2D red_lut : register(s3);
sampler2D green_lut : register(s4);
sampler2D blue_lut : register(s5);
sampler2D blue_noise_sampler : register(s6);

float2 scene_pixel_size : register(c60);
float2 bloom_texel_size : register(c61);
float4 bloom_global_scale_threshold : register(c62);
float3 bloom_dirt_scale : register(c63);
float4 exposure_color_filter_saturation : register(c64);
float2 vignette_end_start : register(c65);
float4 randomness : register(c66);
float3 bloom_local_scale : register(c70);

const static float3 luma_weights = {0.2126, 0.7152, 0.0722};
const static float3 fxaa_luma_weights = {0.299, 0.587, 0.114};
const static float3 saturation_weights = {0.25, 0.5, 0.25};

const static float3 bloom_global_scale = bloom_global_scale_threshold.xyz;
const static float bloom_threshold = bloom_global_scale_threshold.w;
const static float bloom_radius_scale = 1.0;

const static float vignette_end = vignette_end_start.x;
const static float vignette_start = vignette_end_start.y;

const static float3 exposure_color_filter = exposure_color_filter_saturation.xyz;
const static float saturation = exposure_color_filter_saturation.w;

const static bool bloom = BLOOM_ACTIVE;
const static bool bloom_use_dirt = BLOOM_USE_DIRT;
const static bool stock_hdr = STOCK_HDR_STATE;
const static bool vignette = VIGNETTE_ACTIVE;

float3 bloom_box13_downsample(sampler2D samp, float2 texcoords)
{
   const static float2 ts = bloom_texel_size;

   float3 inner = 0.0;
   inner += tex2D(samp, texcoords + float2(-1.0, -1.0) * ts).rgb;
   inner += tex2D(samp, texcoords + float2(1.0, -1.0) * ts).rgb;
   inner += tex2D(samp, texcoords + float2(-1.0, 1.0) * ts).rgb;
   inner += tex2D(samp, texcoords + float2(1.0, 1.0) * ts).rgb;

   float3 A = tex2D(samp, texcoords + float2(-2.0, -2.0) * ts).rgb;
   float3 B = tex2D(samp, texcoords + float2(0.0, -2.0) * ts).rgb;
   float3 C = tex2D(samp, texcoords + float2(2.0, -2.0) * ts).rgb;
   float3 D = tex2D(samp, texcoords + float2(2.0, 0.0) * ts).rgb;
   float3 E = tex2D(samp, texcoords + float2(2.0, 2.0) * ts).rgb;
   float3 F = tex2D(samp, texcoords + float2(0.0, 2.0) * ts).rgb;
   float3 G = tex2D(samp, texcoords + float2(-2.0, 2.0) * ts).rgb;
   float3 H = tex2D(samp, texcoords + float2(-2.0, 0.0) * ts).rgb;
   float3 I = tex2D(samp, texcoords).rgb;

   float3 color = inner * (0.25 * 0.5);
   color += (A + B + H + I) * (0.25 * 0.125);
   color += (B + C + I + D) * (0.25 * 0.125);
   color += (I + D + F + E) * (0.25 * 0.125);
   color += (H + I + G + F) * (0.25 * 0.125);

   return color;
}

float3 bloom_tent9_upsample(sampler2D samp, float2 texcoords)
{
   const static float2 ts = bloom_texel_size;
   const static float scale = bloom_radius_scale;

   float3 color = 0.0;
   color += tex2D(samp, texcoords).rgb * 0.25;

   color += tex2D(samp, texcoords + float2(-1.0, -1.0) * ts * scale).rgb * 0.0625;
   color += tex2D(samp, texcoords + float2(0.0, -1.0) * ts * scale).rgb * 0.125;
   color += tex2D(samp, texcoords + float2(1.0, -1.0) * ts * scale).rgb * 0.0625;
   color += tex2D(samp, texcoords + float2(1.0, 0.0) * ts * scale).rgb * 0.125;
   color += tex2D(samp, texcoords + float2(1.0, 1.0) * ts * scale).rgb * 0.0625;
   color += tex2D(samp, texcoords + float2(0.0, 1.0) * ts * scale).rgb * 0.125;
   color += tex2D(samp, texcoords + float2(-1.0, 1.0) * ts * scale).rgb * 0.0625;
   color += tex2D(samp, texcoords + float2(-1.0, 0.0) * ts * scale).rgb * 0.125;

   return color * bloom_local_scale;
}

#endif