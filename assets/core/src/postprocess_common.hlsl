#ifndef POSTPROCESS_COMMON_INCLUDED
#define POSTPROCESS_COMMON_INCLUDED

Texture2D<float4> scene_texture : register(ps_3_0, s0);
Texture2D<float4> bloom_texture : register(ps_3_0, s1);
Texture2D<float3> dirt_texture : register(ps_3_0, s2);
Texture3D<float3> color_grading_lut : register(ps_3_0, s3);
Texture2D<float3> blue_noise_texture : register(ps_3_0, s4);

sampler2D fxaa_scene_sampler : register(ps_3_0, s0);

SamplerState linear_clamp_sampler;
SamplerState linear_wrap_sampler;

float4 scene_pixel_metrics : register(c60);
float2 bloom_texel_size : register(c61);
float4 bloom_global_scale_threshold : register(c62);
float3 bloom_dirt_scale : register(c63);
float3 exposure_vignette_end_start : register(c64);
float4 film_grain_params : register(c65);
float4 randomness : register(c66);
float3 bloom_local_scale : register(c70);

const static float2 scene_pixel_size = scene_pixel_metrics.xy;
const static float2 scene_resolution = scene_pixel_metrics.zw;

const static float3 luma_weights = {0.2126, 0.7152, 0.0722};
const static float3 fxaa_luma_weights = {0.299, 0.587, 0.114};
const static uint color_grading_lut_size = 32;

const static float3 bloom_global_scale = bloom_global_scale_threshold.xyz;
const static float bloom_threshold = bloom_global_scale_threshold.w;
const static float bloom_radius_scale = 1.0;

const static float exposure = exposure_vignette_end_start.x;

const static float vignette_end = exposure_vignette_end_start.y;
const static float vignette_start = exposure_vignette_end_start.z;

const static float film_grain_amount = film_grain_params.x;
const static float film_grain_size = film_grain_params.y;
const static float film_grain_color_amount = film_grain_params.z;
const static float film_grain_luma_amount = film_grain_params.w;

const static bool bloom = BLOOM_ACTIVE;
const static bool bloom_use_dirt = BLOOM_USE_DIRT;
const static bool stock_hdr = STOCK_HDR_STATE;
const static bool vignette = VIGNETTE_ACTIVE;
const static bool film_grain = FILM_GRAIN_ACTIVE;
const static bool film_grain_colored = FILM_GRAIN_COLORED;

float3 bloom_box13_downsample(Texture2D<float4> tex, float2 texcoords)
{
   const static float2 ts = bloom_texel_size;

   float3 inner = 0.0;
   inner += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-1.0, -1.0) * ts, 0).rgb;
   inner += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(1.0, -1.0) * ts, 0).rgb;
   inner += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-1.0, 1.0) * ts, 0).rgb;
   inner += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(1.0, 1.0) * ts, 0).rgb;

   float3 A = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-2.0, -2.0) * ts, 0).rgb;
   float3 B = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(0.0, -2.0) * ts, 0).rgb;
   float3 C = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(2.0, -2.0) * ts, 0).rgb;
   float3 D = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(2.0, 0.0) * ts, 0).rgb;
   float3 E = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(2.0, 2.0) * ts, 0).rgb;
   float3 F = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(0.0, 2.0) * ts, 0).rgb;
   float3 G = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-2.0, 2.0) * ts, 0).rgb;
   float3 H = tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-2.0, 0.0) * ts, 0).rgb;
   float3 I = tex.SampleLevel(linear_clamp_sampler, texcoords, 0.0).rgb;

   float3 color = inner * (0.25 * 0.5);
   color += (A + B + H + I) * (0.25 * 0.125);
   color += (B + C + I + D) * (0.25 * 0.125);
   color += (I + D + F + E) * (0.25 * 0.125);
   color += (H + I + G + F) * (0.25 * 0.125);

   return color;
}

float3 bloom_tent9_upsample(Texture2D<float4> tex, float2 texcoords)
{
   const static float2 ts = bloom_texel_size;
   const static float scale = bloom_radius_scale;

   float3 color = 0.0;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords, 0).rgb * 0.25;

   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-1.0, -1.0) * ts * scale, 0).rgb * 0.0625;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(0.0, -1.0) * ts * scale, 0).rgb * 0.125;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(1.0, -1.0) * ts * scale, 0).rgb * 0.0625;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(1.0, 0.0) * ts * scale, 0).rgb * 0.125;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(1.0, 1.0) * ts * scale, 0).rgb * 0.0625;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(0.0, 1.0) * ts * scale, 0).rgb * 0.125;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-1.0, 1.0) * ts * scale, 0).rgb * 0.0625;
   color += tex.SampleLevel(linear_clamp_sampler, texcoords + float2(-1.0, 0.0) * ts * scale, 0).rgb * 0.125;

   return color * bloom_local_scale;
}

#endif