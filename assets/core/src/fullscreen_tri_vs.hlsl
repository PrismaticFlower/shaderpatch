
float4 main_vs(float2 position : POSITION, inout float2 texcoords : TEXCOORD,
               uniform float2 pixel_offset : register(c110)) : SV_Position
{
   return float4(position + pixel_offset, 0.0, 1.0);
}
