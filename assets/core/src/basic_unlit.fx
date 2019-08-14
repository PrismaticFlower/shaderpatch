#include "adaptive_oit.hlsl" 
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"

// Textures
Texture2D<float4> color_map : register(ps, t6);
Texture2D<float3> emissive_map : register(ps, t7);

// Game Constants

const static float4 blend_constant = ps_custom_constants[0];
const static float4 x_texcoords_transform = custom_constants[1];
const static float4 y_texcoords_transform = custom_constants[2];

cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   bool   use_emissive_map;
   float  emissive_power;
};

// Shader Feature Controls
const static bool use_transparency = BASIC_UNLIT_USE_TRANSPARENCY;
const static bool use_hardedged_test = BASIC_UNLIT_USE_HARDEDGED_TEST;

struct Vs_output
{
   float2 texcoords : TEXCOORDS;
   
   float fade : FADE;
   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;
   
   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);   

   float near_fade;
   calculate_near_fade_and_fog(positionWS, positionPS, near_fade, output.fog);

   output.fade = saturate(near_fade);

   return output;
}

float4 main_ps(Vs_output input) : SV_Target0
{
   const float2 texcoords = input.texcoords;

   float4 color = color_map.Sample(aniso_wrap_sampler, texcoords);

   // Hardedged Alpha Test
   if (use_hardedged_test && color.a < 0.5) discard;

   const float3 emissive = emissive_map.Sample(aniso_wrap_sampler, texcoords) * emissive_power;

   color.rgb += use_emissive_map ? emissive : 0.0;
   color.rgb *= lighting_scale;

   // Apply fog.
   color.rgb = apply_fog(color.rgb, input.fog);

   if (use_transparency) {
      color.a = lerp(1.0, color.a, blend_constant.b);
      color.a = saturate(color.a * input.fade);

      color.rgb /= max(color.a, 1e-5);
   }
   else {
      color.a = input.fade;
   }
   
   return color;
}

[earlydepthstencil]
void oit_main_ps(Vs_output input, uint coverage : SV_Coverage)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)input.positionPS.xy, input.positionPS.z, coverage, color);
}
