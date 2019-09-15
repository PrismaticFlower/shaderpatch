#ifndef SCOPE_BLUR_COMMON_INCLUDED
#define SCOPE_BLUR_COMMON_INCLUDED

#pragma warning(disable : 4000)

const static uint scope_blur_sample_count = 12;
const static float scope_blur_angle = 0.001;

float scope_blur_fade(const float2 texcoords)
{
   return smoothstep(0.1, 0.5, distance(texcoords, float2(0.5, 0.5)));
}

float2x2 scope_blur_matrix(const uint i)
{
   const float angle = i * scope_blur_angle;
   const float2x2 mat = {{cos(angle), -sin(angle)}, {sin(angle), cos(angle)}};

   return mat;
}

float4 scope_blur(float2 texcoords, uint mip, Texture2D<float4> src_texture,
                  SamplerState samp)
{
   const float fade = scope_blur_fade(texcoords);

   [branch] if (fade <= 0.0)
   {
      return 0.0;
   }

   float3 color = 0.0;

   [unroll] for (uint i = 0; i < scope_blur_sample_count; ++i)
   {
      const float2x2 mat = scope_blur_matrix(i);

      texcoords = mul(texcoords - 0.5, mat) + 0.5;

      color += src_texture.SampleLevel(samp, texcoords, mip).rgb;
   }

   color /= (float)scope_blur_sample_count;

   return float4(color, fade);
}

#pragma warning(enable : 4000)

#endif