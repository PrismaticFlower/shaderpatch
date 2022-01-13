
#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Textures
Texture2D<float3> blur_buffer : register(t1);
Texture2D<float4> color_map : register(t7);

// Game Custom Constants

const static float2 fade_factor = custom_constants[0].xy;
const static float4 texcoords_transform = custom_constants[1];


cbuffer MaterialConstants : register(MATERIAL_CB_INDEX)
{
   bool   use_aniso_wrap_sampler;
   float  brightness_scale;
};

struct Vs_output
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();
   float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;

   output.texcoords = input.texcoords() * texcoords_transform.xy + texcoords_transform.zw;

   if (particle_texture_scale) {
      output.texcoords = 
         (input.texcoords() * 32767.0 * (1.0 / 2048.0)) * texcoords_transform.xy + texcoords_transform.zw;
   }

   const float near_fade = calculate_near_fade_transparent(positionPS);
   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   float4 color = get_material_color(input.color());
   
   color.a *= (near_fade * fade_scale);
   color.rgb = color.rgb * color.a * lighting_scale;

   output.color = color;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
};

float4 main_ps(Ps_input input) : SV_Target0
{
   float4 texture_color = 0.0;
   [branch]
   if (use_aniso_wrap_sampler) {
      texture_color = color_map.Sample(aniso_wrap_sampler, input.texcoords);
   }
   else {
      texture_color = color_map.Sample(linear_clamp_sampler, input.texcoords);
   }

   float4 color = input.color * texture_color;

   color.rgb *= brightness_scale;
   color.rgb = apply_fog(color.rgb, input.fog);

   return float4(color.rgb / max(color.a, 1e-5), color.a);
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, float4 positionSS : SV_Position, uint coverage : SV_Coverage)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color, coverage);
}

float4 blur_ps(Ps_input input, float4 positionSS : SV_Position) : SV_Target0
{
   const float2 scene_texcoords = positionSS.xy * rt_resolution.zw;
   const float3 scene_color = blur_buffer.SampleLevel(linear_clamp_sampler,
                                                      scene_texcoords,
                                                      0);
   
   const float alpha = color_map.Sample(linear_clamp_sampler, input.texcoords).a;

   float3 color = scene_color * input.color.rgb;

   color = apply_fog(color, input.fog);

   return float4(color, saturate(alpha * input.color.a));
}

[earlydepthstencil]
void oit_blur_ps(Ps_input input, float4 positionSS : SV_Position, uint coverage : SV_Coverage)
{
   const float4 color = blur_ps(input, positionSS);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color, coverage);
}