
#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"

Texture2D<float4> diffuse_map : register(ps_3_0, s0);
Texture2D<float4> bump_map : register(ps_3_0, s1);

SamplerState linear_wrap_sampler;

float4 decal_constants[2] : register(ps, c[0]);

struct Vs_input
{
   float3 position : POSITION;
   unorm float4 color : COLOR;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float4 positionPS : SV_Position;
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
};

Vs_output decal_vs(Vs_input input)
{
   Vs_output output;

   float3 positionWS = mul(float4(input.position, 1.0), world_matrix);

   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);
   output.texcoords = input.texcoords;

   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   float4 color;
   color.rgb = input.color.rgb * material_diffuse_color.rgb;
   color.rgb *= hdr_info.rgb;
   color.a = material_diffuse_color.a * near_scene.fade;

   output.color = get_material_color(input.color);

   return output;
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : TEXCOORD1;
};

float4 diffuse_ps(Ps_input input) : SV_Target0
{
   float4 diffuse_color = diffuse_map.Sample(linear_wrap_sampler, input.texcoords);

   float3 color = diffuse_color.rgb * input.color.rgb;

   color = lerp(decal_constants[0].rgb, color, diffuse_color.a * input.color.a);

   return float4(color, decal_constants[0].a);
}

float4 diffuse_bump_ps(Ps_input input) : SV_Target0
{
   float4 diffuse_color = diffuse_map.Sample(linear_wrap_sampler, input.texcoords);
   float4 bump_color = bump_map.Sample(linear_wrap_sampler, input.texcoords);

   float3 color;

   color = saturate(dot((bump_color - 0.5) * 2.0, (input.color - 0.5) * 2.0));
   color = saturate(color * decal_constants[1].rgb);
   color *= diffuse_color.rgb;

   const float alpha = saturate(bump_color.a * input.color.a * diffuse_color.a);

   color = lerp(decal_constants[0].rgb, color, alpha);

   return float4(color, alpha);
}

float4 bump_ps(Ps_input input) : SV_Target0
{
   const float4 bump_color = diffuse_map.Sample(linear_wrap_sampler, input.texcoords);

   float3 color;
   color = saturate(dot((bump_color.rgb - 0.5) * 2.0, (input.color.rgb - 0.5) * 2.0));

   const float alpha_factor = saturate(bump_color.a * input.color.a);

   color = lerp(decal_constants[0].rgb, color.rgb, alpha_factor);

   return float4(color, decal_constants[0].a);
}