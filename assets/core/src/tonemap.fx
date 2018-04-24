
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD) : POSITION
{
   return float4(position, 0.0, 1.0);
}


float4 linear_ps(float2 texcoords : TEXCOORD, sampler2D input)
{
   return tex2D(input);
}