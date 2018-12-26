
#include "constants_list.hlsl"

struct Vs_input {
   float4 position : POSITIONT;
};

struct Vs_output {
   nointerpolation float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vs_input input)
{
   Vs_output output;

   const float2 position = 
      ((input.position.xy * ff_inv_resolution) - 0.5) * float2(2.0, -2.0);

   output.positionPS = float4(position, 0.0, 1.0);
   output.color = ff_texture_factor;
   
   return output;
}

float4 main_ps(nointerpolation float4 color : COLOR) : SV_Target0
{
   return color;
}
