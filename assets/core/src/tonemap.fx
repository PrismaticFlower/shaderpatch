
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD) : POSITION
{
   return float4(position, 0.0, 1.0);
}

sampler2D input_sampler : register(s04);

float4 linear_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return tex2D(input_sampler, texcoords);
}

float3 exposure_color_filter : register(c60);
float saturation : register(c61);

const static float log_contrast_midpoint = log2(0.18);
const static float log_contrast_epsilon = 1e-5f;

const static float3 luma_weights = {0.25, 0.5, 0.25};

float sample_lut(sampler2D lut, float v)
{
   return tex2D(lut, float2(sqrt(v) + 1.0 / 256.0, 0.0)).x;
}

float4 color_grading_ps(float2 texcoords : TEXCOORD, sampler2D r_lut : register(s05), 
                        sampler2D g_lut : register(s06), sampler2D b_lut : register(s07)) : COLOR
{
   float3 color = tex2D(input_sampler, texcoords).rgb;

   color *= exposure_color_filter;

   const float3 grey = dot(luma_weights, color);
   color = grey + (saturation * (color - grey));

   color.r = sample_lut(r_lut, color.r);
   color.g = sample_lut(g_lut, color.g);
   color.b = sample_lut(b_lut, color.b);

   return float4(color, 1.0);
}