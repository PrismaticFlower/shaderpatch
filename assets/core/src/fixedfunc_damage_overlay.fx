
#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float3> source_texture : register(t0);

const static float3 damage_color = {0.874, 0.125, 0.125};

struct Vs_output {
   nointerpolation float alpha : ALPHA;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) output.positionPS = float4(-1.f, -1.f, 0.0, 1.0);  
   else if (id == 1) output.positionPS = float4(-1.f, 3.f, 0.0, 1.0);  
   else output.positionPS = float4(3.f, -1.f, 0.0, 1.0);
   
   output.alpha = ff_texture_factor.a;
   
   return output;
}

float4 main_ps(nointerpolation float alpha : ALPHA) : SV_Target0
{
   return float4(damage_color, alpha);
}
