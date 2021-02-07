// clang-format off

#include "color_utilities.hlsl"

#define A_GPU 1
#define A_HLSL 1

#if SP_FFX_CAS_FP16

#define A_HALF 1
#define CAS_PACKED_ONLY 1

#endif

cbuffer CASConstants : register(b0)
{
   uint4 const0;
   uint4 const1;
};

Texture2D input_texture : register(t0);
RWTexture2D<float4> output_texture : register(u0);

const static bool use_fp16_path = SP_FFX_CAS_FP16;
const static bool sharpen_only = SP_FFX_CAS_SHARPEN_ONLY;

#include "ffx_a.h"

#if SP_FFX_CAS_FP16

AH3 CasLoadH(ASW2 p)
{
   return (AH3)input_texture[p].rgb;
}

void CasInputH(inout AH2 r, inout AH2 g, inout AH2 b) {}

#else

AF3 CasLoad(ASU2 p)
{
   return input_texture[p].rgb;
}

void CasInput(inout AF1 r, inout AF1 g, inout AF1 b) {}

#endif



#include "ffx_cas.h"

void output(const uint2 xy, const float3 color)
{
   const float3 srgb_color = linear_to_srgb(saturate(color));
   
   output_texture[xy] = float4(srgb_color, 1.0);
}

[numthreads(64, 1, 1)] 
void main_cs(uint3 thread_id : SV_GroupThreadID, uint3 group_id : SV_GroupID)
{
   uint2 xy = ARmp8x8(thread_id.x) + AU2(group_id.xy << 4u);

#  if SP_FFX_CAS_FP16 
      AH4 color0, color1;
      AH2 red, green, blue;

      CasFilterH(red, green, blue, xy, const0, const1, sharpen_only);
      CasDepack(color0, color1, red, green, blue);

      output(xy, color0.rgb);
      output(xy + uint2(8u, 0u), color1.rgb);

      xy.y += 8u;

      CasFilterH(red, green, blue, xy, const0, const1, sharpen_only);
      CasDepack(color0, color1, red, green, blue);

      output(xy, color0.rgb);
      output(xy + uint2(8u, 0u), color1.rgb);
#  else
      AF3 color;

      CasFilter(color.r, color.g, color.b, xy, const0, const1, sharpen_only);
      output(xy, color);

      xy.x += 8u;

      CasFilter(color.r, color.g, color.b, xy, const0, const1, sharpen_only);
      output(xy, color);

      xy.y += 8u;

      CasFilter(color.r, color.g, color.b, xy, const0, const1, sharpen_only);
      output(xy, color);

      xy.x -= 8u;

      CasFilter(color.r, color.g, color.b, xy, const0, const1, sharpen_only);
      output(xy, color);
#  endif
}

