
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"

float2 fade_factor : register(vs, c[CUSTOM_CONST_MIN]);
float4 texcoords_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);
float4x4 blur_projection : register(vs, c[CUSTOM_CONST_MIN + 2]);

Texture2D<float4> particle_texture : register(ps_3_0, s0);
Texture2D<float3> blur_buffer : register(ps_3_0, s1);

SamplerState linear_clamp_sampler;

struct Vs_normal_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
   float fog_eye_distance : DEPTH;
};

Vs_normal_output normal_vs(Vertex_input input)
{
   Vs_normal_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();
   float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.texcoords = input.texcoords() * texcoords_transform.xy + texcoords_transform.zw;

   Near_scene near_scene = calculate_near_scene_fade(positionWS);
   near_scene.fade = saturate(near_scene.fade);
   near_scene.fade = near_scene.fade * near_scene.fade;

   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   output.color.rgb = get_material_color(input.color()).rgb * hdr_info.z;
   output.color.a = (near_scene.fade * fade_scale) * get_material_color(input.color()).a;

   return output;
}

struct Vs_blur_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD0;
   float4 blur_texcoords : TEXCOORD1;
   float4 color : TEXCOORD2;
   float fog_eye_distance : DEPTH;
};

Vs_blur_output blur_vs(Vertex_input input)
{
   Vs_blur_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();
   float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.texcoords = input.texcoords() * texcoords_transform.xy + texcoords_transform.zw;
   output.blur_texcoords = mul(float4(input.normal(), 1.0), blur_projection);

   Near_scene near_scene = calculate_near_scene_fade(positionWS);
   near_scene.fade = saturate(near_scene.fade);
   near_scene.fade = near_scene.fade * near_scene.fade;

   const float fade_scale = saturate(positionPS.w * fade_factor.x + fade_factor.y);

   output.color.rgb = get_material_color(input.color()).rgb;
   output.color.a = (near_scene.fade * fade_scale) * get_material_color(input.color()).a;

   return output;
}

struct Ps_normal_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
   float fog_eye_distance : DEPTH;
};

float4 normal_ps(Ps_normal_input input) : SV_Target0
{
   const float4 diffuse_color = particle_texture.Sample(linear_clamp_sampler, input.texcoords);

   float4 color = diffuse_color * input.color;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

struct Ps_blur_input
{
   float2 texcoords : TEXCOORD0;
   float4 blur_texcoords : TEXCOORD1;
   float4 color : TEXCOORD2;
   float fog_eye_distance : DEPTH;
};

float4 blur_ps(Ps_blur_input input) : SV_Target0
{
   const float2 blur_texcoords = input.blur_texcoords.xy / input.blur_texcoords.w;
   const float3 scene_color = blur_buffer.SampleLevel(linear_clamp_sampler, 
                                                      blur_texcoords,
                                                      0);
   
   const float alpha = particle_texture.Sample(linear_clamp_sampler, input.texcoords).a;

   float3 color = scene_color * input.color.rgb;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, saturate(alpha * input.color.a));
}