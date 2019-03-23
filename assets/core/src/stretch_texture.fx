
Texture2D<float4> input_tex;
Texture2DMS<float4> input_ms_tex;

struct Input_vars
{
   float2 src_start;
   float2 src_end;
};

cbuffer InputVars : register(b0)
{
   Input_vars input_vars;
}

struct Vs_output
{
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) {
      output.texcoords = lerp(input_vars.src_start, input_vars.src_end, float2(0.0, 1.0));
      output.positionPS = float4(-1.f, -1.f, 0.0, 1.0);
   }
   else if (id == 1) {
      output.texcoords = lerp(input_vars.src_start, input_vars.src_end, float2(0.0, -1.0));
      output.positionPS = float4(-1.f, 3.f, 0.0, 1.0);
   }
   else {
      output.texcoords = lerp(input_vars.src_start, input_vars.src_end, float2(2.0, 1.0));
      output.positionPS = float4(3.f, -1.f, 0.0, 1.0);
   }

   return output;
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return input_tex[(uint2)texcoords];
}

float4 main_ms_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return input_ms_tex.sample[0][(uint2)texcoords];
}
