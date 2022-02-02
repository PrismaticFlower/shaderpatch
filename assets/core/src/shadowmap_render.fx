
cbuffer Constants : register(b0)
{
   float4x4 inv_view_proj_matrix;
   float4x4 shadow_matrices[4];
   float2 target_resolution;
   float shadow_bias;
}

Texture2D<float> input_depth : register(t0);
Texture2DArray<float> shadow_map : register(t1);
SamplerComparisonState shadow_sampler : register(s0);

const static uint shadow_cascade_count = 4;

struct Vs_output {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(uint id : SV_VertexID)
{
   Vs_output output;

   if (id == 0) {
      output.positionPS = float4(-1.f, -1.f, 1.0, 1.0);
      output.texcoords = float2(0.0, 1.0);
   }
   else if (id == 1) {
      output.positionPS = float4(-1.f, 3.f, 1.0, 1.0);
      output.texcoords = float2(0.0, -1.0);
   }
   else {
      output.positionPS = float4(3.f, -1.f, 1.0, 1.0);
      output.texcoords = float2(2.0, 1.0);
   }

   return output;
}

float shadow_cascade_signed_distance(float3 positionLS)
{
   float3 centered_positionLS = (positionLS - 0.5);
   float3 distance = abs(centered_positionLS) - 0.5;

   return max(max(distance.x, distance.y), distance.z);
}

uint select_shadow_map_cascade(float3 positionWS)
{
   uint cascade_index = (shadow_cascade_count - 1);

   [unroll] for (int i = (shadow_cascade_count - 1); i >= 0; --i)
   {
      float3 positionLS = mul(float4(positionWS, 1.0), shadow_matrices[i]).xyz;

      float cascade_signed_distance = shadow_cascade_signed_distance(positionLS);

      if (shadow_cascade_signed_distance(positionLS) <= 0.0) cascade_index = i;
   }

   return cascade_index;
}

float sample_cascaded_shadow_map(float3 positionWS)
{
   uint cascade_index = select_shadow_map_cascade(positionWS);

   float3 positionLS =
      mul(float4(positionWS, 1.0), shadow_matrices[cascade_index]).xyz;

   // TODO: Offset UV by a half texel?

   float shadow = shadow_map.SampleCmpLevelZero(shadow_sampler,
                                                float3(positionLS.xy, cascade_index),
                                                positionLS.z + shadow_bias);

   return shadow;
}

float4 main_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   float2 pixel_coord = texcoords * target_resolution;
   float pixel_depth = input_depth[pixel_coord];

   float4 positionNDC =
      float4(float2(texcoords.x, 1.0 - texcoords.y) * 2.0 - 1.0, pixel_depth, 1.0);
   float4 positionPS = mul(positionNDC, inv_view_proj_matrix);
   float3 positionWS = positionPS.xyz / positionPS.w;

   float shadow = sample_cascaded_shadow_map(positionWS);

   // uint index = select_shadow_map_cascade(positionWS);
   //
   // if (index == 0) return float3(0.0, 0.0, 1.0);
   // if (index == 1) return float3(0.0, 1.0, 0.0);
   // if (index == 2) return float3(1.0, 0.0, 0.0);
   // if (index == 3) return float3(1.0, 1.0, 0.0);

   return shadow;
}
