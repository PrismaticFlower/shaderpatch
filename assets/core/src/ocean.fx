
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

const static float4 x_texcoords_transform = custom_constants[0];
const static float4 y_texcoords_transform = custom_constants[1];

const static float3 light_directionWS = ps_custom_constants[0].xyz;
const static float3 light_color = ps_custom_constants[1].xyz;
const static float3 ambient_color = ps_custom_constants[2].xyz;

Texture2D<float3> water_texture : register(t0);
Texture2D<float4> foam_texture : register(t1);

struct Vs_output
{
   float3 normalWS : NORMALWS;
   float4 color : COLOR;
   float2 texcoords : TEXCOORD0;
   float fade : FADE;
   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_output near_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   output.color = get_material_color(input.color());
   output.fade = calculate_near_fade(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

Vs_output far_vs(Vertex_input input)
{
   Vs_output output = near_vs(input);

   output.fade = 1.0;

   return output;
}

struct Ps_input
{
   float3 normalWS : NORMALWS;
   float4 color : COLOR;
   float2 texcoords : TEXCOORD0;
   float fade : FADE;
   float fog : FOG;
};

float4 main_ps(Ps_input input) : SV_Target0
{
   const float3 water_color = water_texture.Sample(aniso_wrap_sampler, input.texcoords);
   const float4 foam_color = foam_texture.Sample(aniso_wrap_sampler, input.texcoords);

   float light_intensity = max(dot(normalize(input.normalWS), light_directionWS.xyz), 0.0);

   float3 color = water_color * input.color.rgb;

   color *= (light_intensity * light_color + ambient_color);

   const float foam_blend = foam_color.a * input.color.a;

   color = lerp(color, foam_color.rgb, foam_blend);
   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}
