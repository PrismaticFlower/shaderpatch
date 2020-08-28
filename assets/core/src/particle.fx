
#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

const static float2 fade_factor = custom_constants[0].xy;
const static float4 texcoords_transform = custom_constants[1];

Texture2D<float4> particle_texture : register(t0);
Texture2D<float3> blur_buffer : register(t1);

struct Vs_normal_output
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_normal_output normal_vs(Vertex_input input)
{
   Vs_normal_output output;

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

   output.color.rgb = get_material_color(input.color()).rgb * lighting_scale;
   output.color.a = (near_fade * fade_scale) * get_material_color(input.color()).a;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_blur_output
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_blur_output blur_vs(Vertex_input input)
{
   Vs_blur_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;

   output.texcoords = input.texcoords() * texcoords_transform.xy + texcoords_transform.zw;
   
   const float near_fade = calculate_near_fade_transparent(positionPS);
   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   output.color.rgb = get_material_color(input.color()).rgb;
   output.color.a = (near_fade * fade_scale) * get_material_color(input.color()).a;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_normal_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
};

float4 normal_ps(Ps_normal_input input) : SV_Target0
{
   const float4 diffuse_color = particle_texture.Sample(linear_clamp_sampler, input.texcoords);

   float4 color = diffuse_color * input.color;

   color.rgb = apply_fog(color.rgb, input.fog);

   return color;
}

struct Ps_blur_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float fog : FOG;
   float4 positionSS : SV_Position;
};

float4 blur_ps(Ps_blur_input input) : SV_Target0
{
   const float2 blur_texcoords = input.positionSS.xy * rt_resolution.zw;
   const float3 scene_color = blur_buffer.SampleLevel(linear_clamp_sampler,
                                                      blur_texcoords,
                                                      0);
   
   const float alpha = particle_texture.Sample(linear_clamp_sampler, input.texcoords).a;

   float3 color = scene_color * input.color.rgb;

   color = apply_fog(color, input.fog);

   return float4(color, saturate(alpha * input.color.a));
}

[earlydepthstencil]
void oit_normal_ps(Ps_normal_input input, float4 positionSS : SV_Position)
{
   const float4 color = normal_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color);
}

[earlydepthstencil]
void oit_blur_ps(Ps_blur_input input)
{
   const float4 color = blur_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}