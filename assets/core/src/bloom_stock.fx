
#include "ext_constants_list.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_utilities.hlsl"

struct Vs_input
{
   float4 position : POSITION;
   float4 texcoord : TEXCOORD;
};

struct Vs_screenspace_output
{
   float4 position : POSITION;
   float2 texcoord : TEXCOORD;
};

struct Vs_bloomfilter_output
{
   float4 position : POSITION;
   float2 texcoords[4] : TEXCOORD0;
};

sampler2D buffer_sampler : register(s0);
sampler2D texlimits_sampler : register(s0);

float4 position_offset : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4 texcoords_offsets : register(vs, c[CUSTOM_CONST_MIN + 2]);

Vs_screenspace_output screenspace_vs(Vs_input input)
{
   Vs_screenspace_output output;

   output.texcoord = decompress_texcoords(input.texcoord);

   float4 position = decompress_position(input.position);

   position.xy = position.xy + -position_offset.zw;
   position.xy *= 2.0;
   position.xy = position.xy * float2(1.0, -1.0) + float2(-1.0, 1.0);
   position.zw = float2(0.5, 1.0);

   output.position = position;

   return output;
}

Vs_bloomfilter_output bloomfilter_vs(Vs_input input)
{
   Vs_bloomfilter_output output;

   float2 texcoords = decompress_texcoords(input.texcoord);

   output.texcoords[0] = texcoords * texcoords_offsets.xy + texcoords_offsets.zw;

   output.texcoords[1].x = texcoords.x * texcoords_offsets.x + -texcoords_offsets.z;
   output.texcoords[1].y = texcoords.y * texcoords_offsets.y + texcoords_offsets.w;

   output.texcoords[2].x = texcoords.x * texcoords_offsets.x + texcoords_offsets.z;
   output.texcoords[2].y = texcoords.y * texcoords_offsets.y + -texcoords_offsets.w;

   output.texcoords[3] = texcoords * texcoords_offsets.xy + -texcoords_offsets.zw;

   float4 position = decompress_position(input.position);

   position.xy = position.xy + -position_offset.zw;
   position.xy *= 2.0;
   position.xy = position.xy * float2(1.0, -1.0) + float2(-1.0, 1.0);
   position.zw = float2(0.5, 1.0);

   output.position = position;

   return output;
}

float4 ps_constants[3] : register(ps, c[0]);

const static float3 luma_weights = ps_constants[0].rgb;
const static float color_bias = ps_constants[0].a;
const static float luma_bias = 0.45;
const static float3 screenspace_tint = ps_constants[2].rgb;
const static float screenspace_alpha = ps_constants[2].a;
const static float blur_weight = ps_constants[1].a;

float4 glowthreshold_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float3 color = tex2D(buffer_sampler, texcoord).rgb;

   float luma = dot(saturate(color - color_bias), luma_weights) + luma_bias;

   if (luma > 0.5) return float4(color, luma);

   return float4(0.0.xxx, luma);
}

float4 luminance_ps(float2 texcoord : TEXCOORD) : COLOR
{
   return dot(tex2D(buffer_sampler, texcoord).rgb, luma_weights);
}

float4 screenspace_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float3 color = tex2D(buffer_sampler, texcoord).rgb;

   return float4(color * screenspace_tint, screenspace_alpha);
}

struct Ps_bloomfilter_input
{
   float2 texcoords[4] : TEXCOORD0;
};

float4 bloomfilter_ps(Ps_bloomfilter_input input) : COLOR
{
   float4 color = 0.0;

   [unroll] for (int i = 0; i < 4; ++i) {
      color += tex2D(buffer_sampler, input.texcoords[i]) * blur_weight;
   }

   return color;
}

float4 screenspace_ex_ps(float2 texcoord : TEXCOORD) : COLOR
{
   float3 color = tex2Dgaussian(buffer_sampler, texcoord, rt_resolution.zw).rgb;

   return float4(color * screenspace_tint, screenspace_alpha);
}

float4 bloomfilter_ex_ps(Ps_bloomfilter_input input) : COLOR
{
   float4 color = 0.0;

   [unroll] for (int i = 0; i < 4; ++i) {
      color += tex2Dgaussian(buffer_sampler, input.texcoords[i], rt_resolution.zw) * blur_weight;
   }

   return color;
}

float4 bloomfilter_ex2_ps(float2 texcoord : TEXCOORD) : COLOR
{
   // 4 sigma
   // const static float centre_weight = 0.0181;
   // 
   // const static float mid_offsets[2] = {1.4766918190223388, 3.4458260763902953};
   // const static float mid_weights[2] = {0.033529000000000003, 0.024688999999999999};
   // 
   // const static float2 corner_offset_00 = {1.4766949493648465, 1.4766949493648467};
   // const static float corner_weight_00 = 0.062111;
   // 
   // const static float2 corner_offset_10 = {3.4458194857442721, 1.4766923211474550};
   // const static float corner_weight_10 = 0.045735999999999999;
   // 
   // const static float2 corner_offset_01 = {1.4766923211474550, 2.4458194857442712};
   // const static float corner_weight_01 = 0.045735999999999999;
   // 
   // const static float2 corner_offset_11 = {3.4458071029813522, 2.4458071029813517};
   // const static float corner_weight_11 = 0.033675999999999998;

   // 3 sigma
   const static float centre_weight = 0.023342;
   
   const static float mid_offsets[2] = {1.4588079664878373, 3.4048455937735373};
   const static float mid_weights[2] = {0.040820999999999996, 0.023897999999999999};
   
   const static float2 corner_offset_00 = {1.4588101808401850, 1.4588101808401852};
   const static float corner_weight_00 = 0.071389000000000008;
   
   const static float2 corner_offset_10 = {3.4048524872586321, 1.4588088914411503};
   const static float corner_weight_10 = 0.041792999999999997;
   
   const static float2 corner_offset_01 = {1.4588088914411503, 3.4048524872586321};
   const static float corner_weight_01 = 0.041792999999999997;
   
   const static float2 corner_offset_11 = {3.4048309968529038, 3.4048309968529038};
   const static float corner_weight_11 = 0.024466999999999999;

   float4 color = tex2D(buffer_sampler, texcoord) * centre_weight;

   color += 
      tex2D(buffer_sampler, texcoord + (mid_offsets[0] * rt_resolution.zw) * float2(1, 0)) * mid_weights[0];
   color +=
      tex2D(buffer_sampler, texcoord + (mid_offsets[1] * rt_resolution.zw) * float2(1, 0)) * mid_weights[1];
   color += 
      tex2D(buffer_sampler, texcoord - (mid_offsets[0] * rt_resolution.zw) * float2(1, 0)) * mid_weights[0];
   color += 
      tex2D(buffer_sampler, texcoord - (mid_offsets[1] * rt_resolution.zw) * float2(1, 0)) * mid_weights[1];
   color +=
      tex2D(buffer_sampler, texcoord + (mid_offsets[0] * rt_resolution.zw) * float2(0, 1)) * mid_weights[0];
   color +=
      tex2D(buffer_sampler, texcoord + (mid_offsets[1] * rt_resolution.zw) * float2(0, 1)) * mid_weights[1];
   color +=
      tex2D(buffer_sampler, texcoord - (mid_offsets[0] * rt_resolution.zw) * float2(0, -1)) * mid_weights[0];
   color +=
      tex2D(buffer_sampler, texcoord - (mid_offsets[1] * rt_resolution.zw) * float2(0, -1)) * mid_weights[1];

   color += tex2D(buffer_sampler, texcoord + (corner_offset_00 * rt_resolution.zw)) * corner_weight_00;
   color += tex2D(buffer_sampler, texcoord + (corner_offset_01 * rt_resolution.zw)) * corner_weight_01;
   color += tex2D(buffer_sampler, texcoord + (corner_offset_10 * rt_resolution.zw)) * corner_weight_10;
   color += tex2D(buffer_sampler, texcoord + (corner_offset_11 * rt_resolution.zw)) * corner_weight_11;

   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_00 * rt_resolution.zw) * float2(-1, 1)) * corner_weight_00;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_01 * rt_resolution.zw) * float2(-1, 1)) * corner_weight_01;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_10 * rt_resolution.zw) * float2(-1, 1)) * corner_weight_10;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_11 * rt_resolution.zw) * float2(-1, 1)) * corner_weight_11;

   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_00 * rt_resolution.zw) * float2(-1, -1)) * corner_weight_00;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_01 * rt_resolution.zw) * float2(-1, -1)) * corner_weight_01;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_10 * rt_resolution.zw) * float2(-1, -1)) * corner_weight_10;
   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_11 * rt_resolution.zw) * float2(-1, -1)) * corner_weight_11;

   color += 
      tex2D(buffer_sampler, texcoord + (corner_offset_00 * rt_resolution.zw) * float2(1, -1)) * corner_weight_00;
   color +=
      tex2D(buffer_sampler, texcoord + (corner_offset_01 * rt_resolution.zw) * float2(1, -1)) * corner_weight_01;
   color +=
      tex2D(buffer_sampler, texcoord + (corner_offset_10 * rt_resolution.zw) * float2(1, -1)) * corner_weight_10;
   color +=
      tex2D(buffer_sampler, texcoord + (corner_offset_11 * rt_resolution.zw) * float2(1, -1)) * corner_weight_11;

   const static float blur_scale = blur_weight * 4.0;

   return color * blur_scale;
}

