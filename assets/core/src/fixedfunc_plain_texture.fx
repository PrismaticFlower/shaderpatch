
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float4> source_texture : register(t0);

struct Vs_input {
   float4 position : POSITIONT;
   float2 texcoords : TEXCOORD;
};

struct Vs_output {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;

   const float2 position = 
      ((input.position.xy * ff_inv_resolution) - 0.5) * float2(2.0, -2.0);

   output.positionPS = float4(position, 0.0, 1.0);
   output.texcoords = input.texcoords;
   
   return output;
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);
}
