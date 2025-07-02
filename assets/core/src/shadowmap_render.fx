
cbuffer Constants : register(b0)
{
   float4x4 inv_view_proj_matrix;
   float4x4 shadow_matrices[4];
   float2 target_resolution;
   float shadow_bias;

   float cascade_fade_distance;
   float inv_cascade_fade_distance;

   float inv_game_intensity;
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

void select_shadow_map_cascade(float3 positionWS, 
                               out uint out_cascade_index,
                               out float out_cascade_signed_distance,
                               out float3 out_positionLS)
{
   uint cascade_index = shadow_cascade_count - 1;

   for (int i = shadow_cascade_count - 2; i >= 0; --i)
   {
      float3 positionLS = mul(float4(positionWS, 1.0), shadow_matrices[i]).xyz;

      float cascade_signed_distance = shadow_cascade_signed_distance(positionLS);

      if (shadow_cascade_signed_distance(positionLS) <= 0.0) {
         cascade_index = i;
         out_cascade_signed_distance = cascade_signed_distance;
         out_positionLS = positionLS;
      }
   }

   if (cascade_index == shadow_cascade_count - 1) {
         out_cascade_signed_distance = -0.5;
         out_positionLS = mul(float4(positionWS, 1.0), shadow_matrices[cascade_index]).xyz;
   }

   out_cascade_index = cascade_index;
}

float sample_shadow_map(Texture2DArray<float> shadow_map, float2 positionLS, float light_depth,
                        uint cascade_index, float2 inv_shadow_map_resolution)
{
   // Sampling pattern from MiniEngine https://github.com/microsoft/DirectX-Graphics-Samples/blob/0aa79bad78992da0b6a8279ddb9002c1753cb849/MiniEngine/Model/Shaders/Lighting.hlsli#L70

   const float dilation = 2.0;

   float d1 = dilation * inv_shadow_map_resolution.x * 0.125;
   float d2 = dilation * inv_shadow_map_resolution.x * 0.875;
   float d3 = dilation * inv_shadow_map_resolution.x * 0.625;
   float d4 = dilation * inv_shadow_map_resolution.x * 0.375;

   float shadow =
      (2.0 * shadow_map.SampleCmpLevelZero(shadow_sampler, float3(positionLS.xy, cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d2, d1), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d1, -d2), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d2, -d1), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d1, d2), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d4, d3), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(-d3, -d4), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler,
                                     float3(positionLS.xy + float2(d4, -d3), cascade_index), light_depth).r +
       shadow_map.SampleCmpLevelZero(shadow_sampler, float3(positionLS.xy + float2(d3, d4), cascade_index),
                                     light_depth).r) /
      10.0;

   return shadow;
}


float sample_cascaded_shadow_map(float3 positionWS)
{
   uint cascade_index;
   float cascade_signed_distance;
   float3 positionLS;

   select_shadow_map_cascade(positionWS, cascade_index, cascade_signed_distance, positionLS);

   // TODO: Offset UV by a half texel?

   float shadow = sample_shadow_map(shadow_map, positionLS.xy, positionLS.z + shadow_bias, 
                                    cascade_index, 1.0 / 2048.0);
   
   [branch]
   if (cascade_index != 3 && cascade_signed_distance >= cascade_fade_distance) {
      const uint next_cascade_index = cascade_index + 1;
      const float fade = cascade_signed_distance * inv_cascade_fade_distance;
      
      float3 next_positionLS = mul(float4(positionWS, 1.0), shadow_matrices[next_cascade_index]).xyz;

      float next_shadow = sample_shadow_map(shadow_map, next_positionLS.xy, next_positionLS.z + shadow_bias, 
                                            next_cascade_index, 1.0 / 2048.0);

      shadow = lerp(next_shadow, shadow, smoothstep(0.0, 1.0, saturate(fade)));
   }

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

float4 main_intensity_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   return (1.0 - (1.0 - main_ps(texcoords)) *  inv_game_intensity);
}