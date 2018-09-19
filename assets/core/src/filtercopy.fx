#include "constants_list.hlsl"

#if !defined(TEXTURE_COUNT) || (TEXTURE_COUNT > 4 || TEXTURE_COUNT < 1)
#error TEXTURE_COUNT must be defined to 1, 2, 3 or 4.
#endif

float4 texcoord_offsets[TEXTURE_COUNT] : register(vs, c[CUSTOM_CONST_MIN]);

Texture2D<float4> textures[TEXTURE_COUNT]: register(ps_3_0, s0);

SamplerState linear_clamp_sampler;

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float4 positionPS : SV_Position;
   float2 texcoords[TEXTURE_COUNT] : TEXCOORD0;
};

Vs_output filtercopy_vs(Vs_input input)
{
   Vs_output output;

   output.positionPS = float4(input.position.xy, 0.5, 1.0);

   [unroll] for (int i = 0; i < TEXTURE_COUNT; ++i) {
      output.texcoords[i] = input.texcoords + texcoord_offsets[i].xy;
   }

   return output;
}

float4 color_filters[TEXTURE_COUNT] : register(ps, c[0]);

float4 filtercopy_ps(float2 texcoords[TEXTURE_COUNT] : TEXCOORD0) : SV_Target0
{
   float4 result = 0.0;

   [unroll] for (int i = 0; i < TEXTURE_COUNT; ++i) {
      result += textures[i].SampleLevel(linear_clamp_sampler, texcoords[i], 0) * color_filters[i];
   }

   return result;
}