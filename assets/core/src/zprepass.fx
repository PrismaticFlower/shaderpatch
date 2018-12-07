#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_sampler_states.hlsl"

const static float4 x_texcoord_transform = custom_constants[0];
const static float4 y_texcoord_transform = custom_constants[1];

Texture2D<float4> diffuse_map : register(t0);

float4 opaque_vs(Vertex_input input) : SV_Position
{
   Transformer transformer = create_transformer(input);

   return transformer.positionPS();
}

struct Vs_output
{
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

float4 opaque_ps() : SV_Target0
{
   return float4(0.0, 0.0, 0.0, 1.0);
}

float4 hardedged_ps(float2 texcoords : TEXCOORD) : SV_Target0
{
   const float alpha =
      diffuse_map.Sample(aniso_wrap_sampler, texcoords).a * material_diffuse_color.a;

   if (alpha < 0.5) discard;

   return float4(0.0, 0.0, 0.0, 1.0);
}
