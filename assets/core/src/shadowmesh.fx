cbuffer TransformCB : register(b0)
{
   float3 position_decompress_min;
   uint skin_index;
   float3 position_decompress_max;
   float4x3 world_matrix;
   float4 x_texcoord_transform;
   float4 y_texcoord_transform;
}

cbuffer CameraCB : register(b1)
{
   float4x4 view_proj_matrices[4];
}

struct Skin {
   float4x3 matrices[15];
};

struct Vertex_input {
   float3 position()
   {
#ifdef __VERTEX_INPUT_POSITION__
#ifdef __VERTEX_INPUT_IS_COMPRESSED__
      const float3 position = float3(_position.xyz);

      return position_decompress_max.xyz + (position_decompress_min.xyz * position);
#else
      return _position;
#endif
#else
      return float3(0.0, 0.0, 0.0);
#endif
   }

   int bone_index()
   {
#ifdef __VERTEX_INPUT_BLEND_INDICES__
      return int(_blend_indices.x * 255);
#else
      return 0;
#endif
   }

   float2 texcoords()
   {
#ifdef __VERTEX_INPUT_TEXCOORDS__
#ifdef __VERTEX_INPUT_IS_COMPRESSED__
      return (float2)_texcoords / 2048.0;
#else
      return _texcoords;
#endif
#else
      return float2(0.0, 0.0);
#endif
   }

#ifdef __VERTEX_INPUT_POSITION__

#ifdef __VERTEX_INPUT_IS_COMPRESSED__
   int4 _position : POSITION;
#else
   float3 _position : POSITION;
#endif

#endif

#ifdef __VERTEX_INPUT_BLEND_INDICES__
   unorm float4 _blend_indices : BLENDINDICES;
#endif

#ifdef __VERTEX_INPUT_TEXCOORDS__

#ifdef __VERTEX_INPUT_IS_COMPRESSED__
   int2 _texcoords : TEXCOORD;
#else
   float2 _texcoords : TEXCOORD;
#endif

#endif
};

StructuredBuffer<Skin> skins : register(t0);
Texture2D<float4> cutout_map : register(t0);
SamplerState texture_sampler : register(s0);

float3 get_positionOS(Vertex_input input)
{
#ifdef __VERTEX_INPUT_BLEND_INDICES__
   return mul(float4(input.position(), 1.0),
              skins[skin_index].matrices[input.bone_index()]);
#else
   return input.position();
#endif
}

// clang-format off

float4 opaque_vs(Vertex_input input, uint instance : SV_InstanceID,
                 out uint rendertarget : SV_RenderTargetArrayIndex) : SV_Position
{
   // clang-format on

   float3 positionOS = get_positionOS(input);
   float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   float4 positionPS = mul(float4(positionWS, 1.0), view_proj_matrices[instance]);

   rendertarget = instance;

   return positionPS;
}

struct Vs_output {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
   uint rendertarget : SV_RenderTargetArrayIndex;
};

Vs_output hardedged_vs(Vertex_input input, uint instance : SV_InstanceID)
{
   float3 positionOS = get_positionOS(input);
   float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   float4 positionPS = mul(float4(positionWS, 1.0), view_proj_matrices[instance]);

   float2 texcoords =
      float2(dot(float4(input.texcoords(), 0.0, 1.0), x_texcoord_transform),
             dot(float4(input.texcoords(), 0.0, 1.0), y_texcoord_transform));

   Vs_output output;

   output.positionPS = positionPS;
   output.texcoords = texcoords;
   output.rendertarget = instance;

   return output;
}

void hardedged_ps(float2 texcoords : TEXCOORD)
{
   const float alpha = cutout_map.Sample(texture_sampler, texcoords).a;

   if (alpha < 0.5) discard;
}