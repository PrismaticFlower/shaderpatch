#ifndef COLOR_GRADING_INCLUDED
#define COLOR_GRADING_INCLUDED

// clang-format off

float3 logc_to_linear(float3 color)
{
   const float cut = 0.011361f;
   const float a = 5.555556f;
   const float b = 0.047996f;
   const float c = 0.244161f;
   const float d = 0.386036f;
   const float e = 5.301883f;
   const float f = 0.092814f;

   return (pow(10.0f, (color - d) / c) - b) / a;
}

float3 rgb_to_hsv(float3 rgb)
{
   float k = 0.0f;

   [flatten] 
   if (rgb.g < rgb.b) {
      rgb.gb = rgb.bg;
      k = -1.0f;
   }

   [flatten] 
   if (rgb.r < rgb.g) {
      rgb.rg = rgb.gr;
      k = -2.0f / 6.0f - k;
   }

   const float chroma = rgb.r - (rgb.g < rgb.b ? rgb.g : rgb.b);

   float3 hsv;

   hsv.x = abs(k + (rgb.g - rgb.b) / (6.0f * chroma + 1e-20f));
   hsv.y = chroma / (rgb.r + 1e-20f);
   hsv.z = rgb.r;

   return hsv;
}

float3 hsv_to_rgb(float3 hsv)
{
   float3 rgb;

   rgb.r = abs(hsv.x * 6.0f - 3.0f) - 1.0f;
   rgb.g = 2.0f - abs(hsv.x * 6.0f - 2.0f);
   rgb.b = 2.0f - abs(hsv.x * 6.0f - 4.0f);

   rgb = saturate(rgb);

   return ((rgb - 1.0f) * hsv.y + 1.0f) * hsv.z;
}

float3 apply_basic_saturation(float3 color, const float saturation)
{
   const float3 saturation_weights = {0.25f, 0.5f, 0.25f};
   const float grey = dot(saturation_weights, color);

   return grey + (saturation * (color - grey));
}

float3 apply_hsv_adjust(float3 color, const float3 hsv_adjust)
{
   float3 hsv = rgb_to_hsv(color);

   hsv.x = saturate(hsv.x + hsv_adjust.x);
   hsv.y = saturate(hsv.y * hsv_adjust.y);
   hsv.z *= hsv_adjust.z;

   return hsv_to_rgb(hsv);
}

float3 apply_channel_mixing(float3 color, float3 mix_red, float3 mix_green, float3 mix_blue)
{
   return float3(dot(color, mix_red), dot(color, mix_green), dot(color, mix_blue));
}

float3 apply_log_contrast(float3 color, float contrast)
{
   const float contrast_midpoint = log2(0.18f);
   const float contrast_epsilon = 1e-5f;

   const float3 log_col = log2(color + contrast_epsilon);
   const float3 adj_col = contrast_midpoint + (log_col - contrast_midpoint) * contrast;

   return max(0.0f, exp2(adj_col) - contrast_epsilon);
}

float3 apply_lift_gamma_gain(float3 color, float3 lift_adjust, float3 inv_gamma_adjust, float3 gain_adjust)
{
   color = color * gain_adjust + lift_adjust;

   return sign(color) * pow(abs(color), inv_gamma_adjust);
}

#endif