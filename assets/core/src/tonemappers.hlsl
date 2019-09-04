#ifndef TONEMAPPERS_INCLUDED
#define TONEMAPPERS_INCLUDED

// clang-format off

struct Filmic_curve_segment {
   float offset_x;
   float offset_y;
   float scale_x;
   float scale_y;
   float ln_a;
   float b;
};

struct Filmic_curve {
   float w;
   float inv_w;

   float x0;
   float y0;
   float x1;
   float y1;

   Filmic_curve_segment segments[3];
};

float3 eval_aces_srgb_fitted(float3 color)
{
   // Stephen Hill's ACES sRGB method.

   const float3x3 aces_input_mat = {{0.59719f, 0.35458f, 0.04823f},
                                    {0.07600f, 0.90834f, 0.01566f},
                                    {0.02840f, 0.13383f, 0.83777f}};

   const float3x3 aces_output_mat = {{1.60475f, -0.53108f, -0.07367f},
                                     {-0.10208f,  1.10813f, -0.00605f},
                                     {-0.00327f, -0.07276f,  1.07602f}};

   color = mul(aces_input_mat, color);

   // RRT and ODT fit
   {
      const float3 a = color * (color + 0.0245786f) - 0.000090537f;
      const float3 b = color * (0.983729f * color + 0.4329510f) + 0.238081f;
      color = a / b;
   }

   color = mul(aces_output_mat, color);
   color = saturate(color);
   color *= 1.8f;

   return color;
}

float3 eval_filmic_hejl2015(float3 color, const float whitepoint)
{
   const float4 vh = float4(color, whitepoint);
   const float4 va = (1.425f * vh) + 0.05f;
   const float4 vf = ((vh * va + 0.004f) / (vh * (va + 0.55f) + 0.0491f)) - 0.0821f;

   return float3(vf.r, vf.g, vf.b) / vf.w;
}

float3 eval_reinhard(float3 color)
{
   return color / (color + 1.0f);
}

float eval_filmic_segment(float v, Filmic_curve_segment segm)
{
   const float x0 = (v - segm.offset_x) * segm.scale_x;
   float y0 = 0.0f;

   if (x0 > 0) y0 = exp(segm.ln_a + segm.b * log(x0));

   return y0 * segm.scale_y + segm.offset_y;
}

float eval_filmic(float v, Filmic_curve curve)
{
   const float norm_v = v * curve.inv_w;
   const uint index = (norm_v < curve.x0) ? 0 : ((norm_v < curve.x1) ? 1 : 2);

   return eval_filmic_segment(norm_v, curve.segments[index]);
}

float3 eval_filmic(float3 color, Filmic_curve curve)
{
   return float3(eval_filmic(color.r, curve), 
                 eval_filmic(color.g, curve), 
                 eval_filmic(color.b, curve));
}

#endif