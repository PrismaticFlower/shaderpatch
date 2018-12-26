
struct Input_vars
{
   uint2 dest_start;
   uint2 dest_end;
   uint2 dest_length;

   uint2 src_start;
   uint2 src_length;
};

cbuffer InputVars : register(b0)
{
   Input_vars input_vars;
}

Texture2D<float4> input_tex;
RWTexture2D<unorm float4> output_tex;

[numthreads(8, 8, 1)]
void main(uint3 threadid : SV_DispatchThreadID)
{
   const uint2 dest_index = input_vars.dest_start + threadid.xy;

   if (dest_index.x > input_vars.dest_end.x || dest_index.y > input_vars.dest_end.y) return;

   const uint2 src_index =
      input_vars.src_start + (dest_index - input_vars.dest_start) * input_vars.src_length / input_vars.dest_length;

   output_tex[dest_index] = input_tex[src_index];
}