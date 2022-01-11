
Texture2D input_texture;

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float2 size;

   input_texture.GetDimensions(size.x, size.y);

   return input_texture[texcoords * size];
}
