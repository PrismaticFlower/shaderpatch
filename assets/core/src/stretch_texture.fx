
Texture2D<float4> input_tex;
Texture2DMS<float4> input_ms_tex;

struct Input_vars
{
   uint2 dest_length;
   uint2 src_start;
   uint2 src_length;
};

cbuffer InputVars : register(b0)
{
   Input_vars input_vars;
}

float4 main_vs(uint id : SV_VertexID) : SV_Position
{
   if (id == 0) return float4(-1.f, -1.f, 0.0, 1.0);
   else if (id == 1) return float4(-1.f, 3.f, 0.0, 1.0);
   else return float4(3.f, -1.f, 0.0, 1.0);
}

uint2 calc_src_index(float4 positionSS)
{
   const uint2 dest_index = (uint2)positionSS.xy;
   const uint2 src_index = input_vars.src_start + (dest_index * input_vars.src_length / input_vars.dest_length);

   return src_index;
}

float4 main_ps(float4 positionSS : SV_Position) : SV_Target0
{
   const uint2 src_index = calc_src_index(positionSS);

   return input_tex[src_index];
}

float4 main_ms_ps(float4 positionSS : SV_Position) : SV_Target0
{
   const uint2 src_index = calc_src_index(positionSS);

   return input_ms_tex.sample[0][src_index];
}
