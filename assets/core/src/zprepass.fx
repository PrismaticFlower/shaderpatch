#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "vertex_transformer.hlsl"

const static float4 x_texcoord_transform = custom_constants[0];
const static float4 y_texcoord_transform = custom_constants[1];

Texture2D<float4> diffuse_map : register(t0);

// clang-format off

float4 opaque_vs(Vertex_input input) : SV_Position
{
   Transformer transformer = create_transformer(input);

   return transformer.positionPS();
}

struct Vs_output {
   float2 texcoords : TEXCOORD;
   float4 positionPS : SV_Position;
};

Vs_output hardedged_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   output.positionPS = transformer.positionPS();
   output.texcoords = transformer.texcoords(x_texcoord_transform, y_texcoord_transform);

   return output;
}

void opaque_ps() {}

void hardedged_ps(float2 texcoords : TEXCOORD, out uint coverage : SV_Coverage)
{
   coverage = 0;

   [branch] 
   if (supersample_alpha_test) {
      for (uint i = 0; i < GetRenderTargetSampleCount(); ++i) {
         const float2 sample_texcoords = EvaluateAttributeAtSample(texcoords, i);
         const float alpha = diffuse_map.Sample(aniso_wrap_sampler, sample_texcoords).a;

         const uint visible = alpha >= 0.5;

         coverage |= (visible << i);
      }
   } 
   else {
      const float alpha = diffuse_map.Sample(aniso_wrap_sampler, texcoords).a;
      
      coverage = alpha >= 0.5 ? 0xffffffff : 0;
   }
}