
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"

// Sprite shader. I am unsure what/if the game uses it for, it is listed as
// deprecated in the modtools. This implementation was written before I knew 
// that. It is likely useless and pointless.

Texture2D diffuse_map_texture : register(ps_3_0, s0);

SamplerState linear_clamp_sampler;

struct Vs_output
{
   float4 positionPS : POSITION;
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
};

Vs_output sprite_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.texcoords = input.texcoords();

   float4 material_color = get_material_color(input.color());
   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   output.color.rgb = material_color.rgb;
   output.color.a = saturate(near_scene.fade) * material_color.a;

   return output;
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
};

float4 sprite_ps(Ps_input input) : SV_Target0
{
   const float4 diffuse_map_color =
      diffuse_map_texture.Sample(linear_clamp_sampler, input.texcoords);

   return diffuse_map_color * input.color;
}