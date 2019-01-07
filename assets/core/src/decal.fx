
#include "constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

Texture2D<float4> diffuse_map : register(t0);
Texture2D<float4> bump_map : register(t1);

const static float4 decal_constants[2] =
   {ps_custom_constants[0], ps_custom_constants[1]};

struct Vs_input
{
   float3 position : POSITION;
   unorm float4 color : COLOR;
   float2 texcoords : TEXCOORD;
};

struct Vs_output
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
   float4 positionPS : SV_Position;
};

Vs_output decal_vs(Vs_input input)
{
   Vs_output output;

   const float3 positionWS = mul(float4(input.position, 1.0), world_matrix);
   const float4 positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   output.positionPS = positionPS;
   output.texcoords = input.texcoords;

   float near_fade, fog;
   calculate_near_fade_and_fog(positionWS, positionPS, near_fade, fog);

   float4 color;
   color.rgb = input.color.rgb * material_diffuse_color.rgb;
   color.rgb *= lighting_scale;
   color.a = material_diffuse_color.a * near_fade;

   output.color = get_material_color(input.color);

   return output;
}

struct Ps_input
{
   float2 texcoords : TEXCOORD0;
   float4 color : COLOR;
};

float4 diffuse_ps(Ps_input input) : SV_Target0
{
   float4 diffuse_color = diffuse_map.Sample(aniso_wrap_sampler, input.texcoords);

   float3 color = diffuse_color.rgb * input.color.rgb;

   color = lerp(decal_constants[0].rgb, color, diffuse_color.a * input.color.a);

   return float4(color, decal_constants[0].a);
}

float4 diffuse_bump_ps(Ps_input input) : SV_Target0
{
   float4 diffuse_color = diffuse_map.Sample(aniso_wrap_sampler, input.texcoords);
   float4 bump_color = bump_map.Sample(aniso_wrap_sampler, input.texcoords);

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
   const float4 bump_color = diffuse_map.Sample(aniso_wrap_sampler, input.texcoords);

   float3 color;
   color = saturate(dot((bump_color.rgb - 0.5) * 2.0, (input.color.rgb - 0.5) * 2.0));

   const float alpha_factor = saturate(bump_color.a * input.color.a);

   color = lerp(decal_constants[0].rgb, color.rgb, alpha_factor);

   return float4(color, decal_constants[0].a);
}