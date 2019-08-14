#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_sampler_states.hlsl"

TextureCube<float3> skybox_map : register(ps, t6);
TextureCube<float3> skybox_emissive_map : register(ps, t7);

const static bool use_emissive_map = SKYBOX_USE_EMISSIVE_MAP;

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   float emissive_power;
};

struct Vs_output
{
   float3 texcoords : TEXCOORDS;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;
   
   Transformer transformer = create_transformer(input);

   output.texcoords = transformer.positionOS();
   output.positionPS = transformer.positionPS();

   return output;
}

float4 main_ps(Vs_output input) : SV_Target0
{
   float3 color = skybox_map.Sample(linear_wrap_sampler, input.texcoords);

   if (use_emissive_map) {
      color += skybox_emissive_map.Sample(linear_wrap_sampler, input.texcoords) * emissive_power;
   }

   color *= lighting_scale;

   return float4(color, 1.0);
}
