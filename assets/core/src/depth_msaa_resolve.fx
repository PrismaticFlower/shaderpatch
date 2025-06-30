
Texture2DMS<float> ms_input;

const static uint sample_count = SAMPLE_COUNT;

float4 main_vs(uint id : SV_VertexID) : SV_Position
{
   if (id == 0) return float4(-1.f, -1.f, 0.0, 1.0);
   else if (id == 1) return float4(-1.f, 3.f, 0.0, 1.0);
   else return float4(3.f, -1.f, 0.0, 1.0);
}

float main_ps(float4 positionSS : SV_Position) : SV_Depth
{
   float depths[sample_count];

   const uint2 pos = positionSS.xy;

   [unroll] for (uint i = 0; i < sample_count; ++i) {
      depths[i] = ms_input.sample[i][pos];

   }

   float depth = depths[0];

   [unroll] for (i = 1; i < sample_count; ++i) {
      depth = max(depths[i], depth);
   }

   return depth;
}
