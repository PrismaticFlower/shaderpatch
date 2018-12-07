#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

#if !defined(TEXTURE_COUNT) || (TEXTURE_COUNT > 4 || TEXTURE_COUNT < 1)
#error TEXTURE_COUNT must be defined to 1, 2, 3 or 4.
#endif

const static float4 texcoord_offsets[4] = {custom_constants[0], custom_constants[1],
                                           custom_constants[2], custom_constants[3]};

Texture2D<float4> textures[TEXTURE_COUNT]: register(t0);

struct Vs_input
{
   float3 position : POSITION;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float2 texcoords[TEXTURE_COUNT] : TEXCOORD0;
   float4 positionPS : SV_Position;
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

const static float4 color_filters[4] = {ps_custom_constants[0], ps_custom_constants[1], 
                                        ps_custom_constants[2], ps_custom_constants[3]};

float4 filtercopy_ps(float2 texcoords[TEXTURE_COUNT] : TEXCOORD0) : SV_Target0
{
   float4 result = 0.0;

   [unroll] for (int i = 0; i < TEXTURE_COUNT; ++i) {
      result += textures[i].SampleLevel(linear_clamp_sampler, texcoords[i], 0) * color_filters[i];
   }

   return result;
}