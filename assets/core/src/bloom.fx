
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD,
               uniform float2 pixel_offset : register(c110)) : POSITION
{
   return float4(position + pixel_offset, 0.0, 1.0);
}

float4 source_metrics : register(c60);
float threshold : register(c61);
float3 scale : register(c62);

const static float3 luma_weights = {0.2126, 0.7152, 0.0722};

sampler2D source : register(s0);

float3 blur_13(float2 texcoords, float2 dir)
{
   const static float weights[] = {0.225586, 0.314209, 0.069824, 0.003174};
   const static float offsets[] = {0.000000, 1.384615, 3.230769, 5.076923};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   for (int i = 1; i < 4; ++i) {
      color += 
         tex2D(source, texcoords + (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
      color +=
         tex2D(source, texcoords - (((offsets[i] * source_metrics.xy)) * dir)).rgb * weights[i];
   }

   return color;
}

float4 threshold_blur_13_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   float3 centre = tex2D(source, texcoords).rgb;

   if (dot(luma_weights, centre) < threshold) return 0.0;

   return float4(blur_13(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_13_x_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_13(texcoords, float2(1.0, 0.0)), 1.0);
}

float4 blur_13_y_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(blur_13(texcoords, float2(0.0, 1.0)), 1.0);
}

float3 blur_17(float2 texcoords, float2 dir)
{
   const static float weights[] = {0.196381, 0.296753, 0.094421, 0.010376, 0.000259};
   const static float offsets[] = {0.000000, 1.411765, 3.294118, 5.176471, 7.058824};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   for (int i = 1; i < 5; ++i) {
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
   const static float weights[] = {0.161180, 0.265682, 0.121771, 0.028652, 0.003167, 0.000137, 0.000001};
   const static float offsets[] = {0.000000, 1.440000, 3.360000, 5.280000, 7.200000, 9.120000, 11.040000};

   float3 color = tex2D(source, texcoords).rgb * weights[0];

   for (int i = 1; i < 7; ++i) {
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


float4 combine_ps(float2 texcoords : TEXCOORD) : COLOR
{
   return float4(tex2D(source, texcoords).rgb * scale, 1.0f);
}
