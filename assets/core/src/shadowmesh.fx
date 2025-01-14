cbuffer MeshCB : register(b0)
{
   float3 position_decompress_mul;
   float3 position_decompress_add;
}

cbuffer ViewCB : register(b1)
{
   float4x4 projection_matrix;
}

Texture2D<float4> cutout_map : register(t0);
SamplerState texture_sampler : register(s0);

struct Vertex_input {
   int position_XCS : POSITION_X;
   int position_YCS : POSITION_Y;
   int position_ZCS : POSITION_Z;

   float4x3 world_matrix : TRANSFORM;

   int3 positionCS()
   {
      return int3(position_XCS, position_YCS, position_ZCS);
   }
};

float4 opaque_vs(Vertex_input input) : SV_Position
{
   float3 positionCS = input.positionCS();
   float3 positionOS = positionCS * position_decompress_mul + position_decompress_add;
   float3 positionWS = mul(float4(positionOS, 1.0), input.world_matrix);
   float4 positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   return positionPS;
}

struct Vertex_input_textured {
   int position_XCS : POSITION_X;
   int position_YCS : POSITION_Y;
   int position_ZCS : POSITION_Z;
   int texcoord_X : TEXCOORD_X;
   int texcoord_Y : TEXCOORD_Y;

   float4x3 world_matrix : TRANSFORM;

   int3 positionCS()
   {
      return int3(position_XCS, position_YCS, position_ZCS);
   }

   int2 texcoords()
   {
      return int2(texcoord_X, texcoord_Y);
   }
};

struct Vs_output_textured {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output_textured hardedged_vs(Vertex_input_textured input)
{
   float3 positionCS = input.positionCS();
   float3 positionOS = positionCS * position_decompress_mul + position_decompress_add;
   float3 positionWS = mul(float4(positionOS, 1.0), input.world_matrix);
   float4 positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   float2 texcoords = input.texcoords() * (1.0 / 2048.0);

   Vs_output_textured output;

   output.positionPS = positionPS;
   output.texcoords = texcoords;

   return output;
}

void hardedged_ps(float2 texcoords : TEXCOORD)
{
   const float alpha = cutout_map.Sample(texture_sampler, texcoords).a;

   if (alpha < 0.5) discard;
}