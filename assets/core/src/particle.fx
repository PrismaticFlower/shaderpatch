
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"

const static float2 fade_factor = custom_constants[0].xy;
const static float4 texcoords_transform = custom_constants[1];
const static float4x4 blur_projection =
   {custom_constants[2], custom_constants[3], custom_constants[4], custom_constants[5]};

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

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);
   near_fade = saturate(near_fade);
   near_fade *= near_fade;

   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   output.color.rgb = get_material_color(input.color()).rgb * lighting_scale;
   output.color.a = (near_fade * fade_scale) * get_material_color(input.color()).a;

   return output;
}

struct Vs_blur_output
{
   float2 texcoords : TEXCOORD0;
   float4 blur_texcoords : TEXCOORD1;
   float4 color : COLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_blur_output blur_vs(Vertex_input input)
{
   Vs_blur_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();
   float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;

   output.texcoords = input.texcoords() * texcoords_transform.xy + texcoords_transform.zw;
   output.blur_texcoords = mul(float4(input.normal(), 1.0), blur_projection);

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);
   near_fade = saturate(near_fade);
   near_fade *= near_fade;

   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   output.color.rgb = get_material_color(input.color()).rgb;
   output.color.a = (near_fade * fade_scale) * get_material_color(input.color()).a;

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
   float4 blur_texcoords : TEXCOORD1;
   float4 color : COLOR;
   float fog : FOG;
};

float4 blur_ps(Ps_blur_input input) : SV_Target0
{
   const float2 blur_texcoords = input.blur_texcoords.xy / input.blur_texcoords.w;
   const float3 scene_color = blur_buffer.SampleLevel(linear_clamp_sampler,
                                                      blur_texcoords,
                                                      0);
   
   const float alpha = particle_texture.Sample(linear_clamp_sampler, input.texcoords).a;

   float3 color = scene_color * input.color.rgb;

   color = apply_fog(color, input.fog);

   return float4(color, saturate(alpha * input.color.a));
}