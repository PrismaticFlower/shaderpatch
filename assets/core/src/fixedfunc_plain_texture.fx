
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float4> source_texture : register(t0);

struct Vs_output {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) {
      output.positionPS = float4(-1.f, -1.f, 0.0, 1.0);
      output.texcoords = float2(0.0, 1.0);
   }
   else if (id == 1) {
      output.positionPS = float4(-1.f, 3.f, 0.0, 1.0);
      output.texcoords = float2(0.0, -1.0);
   }
   else {
      output.positionPS = float4(3.f, -1.f, 0.0, 1.0);
      output.texcoords = float2(2.0, 1.0);
   }
   
   return output;
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return source_texture.SampleLevel(linear_clamp_sampler, texcoords, 0);
}
