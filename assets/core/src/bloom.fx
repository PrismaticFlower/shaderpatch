
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
const static float2 blur_dir_size = source_metrics.xy;
const static float2 source_size = source_metrics.zw;

sampler2D source : register(s0);
sampler2D combine_sources[4] : register(s1);
sampler2D dirt_sampler : register(s5);

float4 threshold_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 color = tex2D(source, texcoords).rgb;

   if (dot(luma_weights, color) < threshold) return 0.0;

   return float4(color, 1.0);
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

