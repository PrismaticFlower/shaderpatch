#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float4> color_map : register(ps, t7);
Texture2D<float3> emissive_map : register(ps, t8);

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
   float4 color : COLOR;

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
   output.color = get_material_color(input.color());
   output.fade =
      use_transparency ? calculate_near_fade_transparent(positionPS) : 
                         calculate_near_fade(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_output {
   float4 out_color : SV_Target0;

#if BASIC_UNLIT_USE_HARDEDGED_TEST
   uint out_coverage : SV_Coverage;
#endif

   void color(float4 color)
   {
      out_color = color;
   }

   void coverage(uint coverage)
   {  
#     if BASIC_UNLIT_USE_HARDEDGED_TEST
         out_coverage = coverage;
#     endif
   }
};

Ps_output main_ps(Vs_output input)
{
   Ps_output output;

   const float2 texcoords = input.texcoords;

   float4 color = color_map.Sample(aniso_wrap_sampler, texcoords);

   color *= input.color;

   // Hardedged Alpha Test
   if (use_hardedged_test) {
      uint coverage = 0;

      [branch] 
      if (supersample_alpha_test) {
         for (uint i = 0; i < GetRenderTargetSampleCount(); ++i) {
            const float2 sample_texcoords = EvaluateAttributeAtSample(input.texcoords, i);
            const float alpha = color_map.Sample(aniso_wrap_sampler, sample_texcoords).a;

            const uint visible = alpha >= 0.5;

            coverage |= (visible << i);
         }
      }
      else {
         coverage = color.a >= 0.5 ? 0xffffffff : 0;
      }

      if (coverage == 0) discard;

      output.coverage(coverage);
   }

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
      color.a = saturate(input.fade);
   }
   
   output.color(color);

   return output;
}

[earlydepthstencil]
void oit_main_ps(Vs_output input)
{
   Ps_output output = main_ps(input);

   aoit::write_pixel((uint2)input.positionPS.xy, input.positionPS.z, output.out_color);
}
