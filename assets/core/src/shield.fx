#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

const static float3 specular_color = custom_constants[0].xyz;
const static float4 shield_constants = custom_constants[3];
const static float2 near_scene_fade_scale = custom_constants[4].xy;

Texture2D<float4> diffuse_texture : register(t0);

struct Vs_output {
   float2 texcoords : TEXCOORD0;
   float3 normalWS : NORMALWS;
   float3 positionWS : POSITIONWS;
   float3 material_color : MATCOLOR;
   float fog : FOG;
   float fade : FADE;

   float4 positionPS : SV_Position;
};

Vs_output shield_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();
   output.positionWS = positionWS;
   output.texcoords = input.texcoords() + shield_constants.xy;

   float fade = calculate_near_fade(positionPS);
   fade = fade * near_scene_fade_scale.x + near_scene_fade_scale.y;
   fade = saturate(fade);
   fade = fade * fade;

   output.fade = fade;
   output.fog = calculate_fog(positionWS, positionPS);
   output.material_color = get_material_color().rgb;

   return output;
}

float angle_alpha(float3 V, float3 R)
{
   const float alpha = material_diffuse_color.a;
   const float VdotR = dot(V, R);
   const float VdotV = max(dot(V, V), shield_constants.w);

   return saturate(-(((VdotR / VdotV) * -0.5 + 0.5) * shield_constants.z) * alpha + alpha);
}

struct Ps_input {
   float2 texcoords : TEXCOORD0;
   float3 normalWS : NORMALWS;
   float3 positionWS : POSITIONWS;
   float3 material_color : MATCOLOR;
   float fog : FOG;
   float fade : FADE;
};

[earlydepthstencil] float4 shield_ps(Ps_input input) : SV_Target0
{
   const float3 normalWS = normalize(input.normalWS);
   const float3 view_normalWS = normalize(input.positionWS - view_positionWS);
   const float3 H = normalize(-light_directional_0_dir.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, H));
   float3 specular = pow(NdotH, 64.0);
   specular *= specular_color.rgb;

   const float2 texcoords = float2(input.texcoords.x, saturate(input.texcoords.y));
   const float4 diffuse_color = diffuse_texture.Sample(linear_wrap_sampler, texcoords);

   const float alpha = angle_alpha(view_normalWS, reflect(view_normalWS, normalWS));

   float3 color = diffuse_color.rgb * input.material_color;

   color = (color * alpha + specular) * lighting_scale;
   color *= saturate(input.fade);

   color = apply_fog(color, input.fog);

   if (diffuse_color.a <= (1.0 / 255.0)) discard;

   return float4(color, 1.0);
}

[earlydepthstencil] void oit_shield_ps(Ps_input input, float4 positionSS
                                       : SV_Position, uint coverage
                                       : SV_Coverage) {
   const float3 normalWS = normalize(input.normalWS);
   const float3 view_normalWS = normalize(input.positionWS - view_positionWS);
   const float3 H = normalize(-light_directional_0_dir.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, H));
   float3 specular = pow(NdotH, 64.0);
   specular *= specular_color.rgb;

   const float2 texcoords = float2(input.texcoords.x, saturate(input.texcoords.y));
   const float4 diffuse_color = diffuse_texture.Sample(linear_wrap_sampler, texcoords);

   if (diffuse_color.a <= (1.0 / 255.0)) discard;

   const float alpha = angle_alpha(view_normalWS, reflect(view_normalWS, normalWS)) *
                       saturate(input.fade);

   float3 color = diffuse_color.rgb * input.material_color;

   color = (color + (specular / max(alpha, 1e-5))) * lighting_scale;
   color = apply_fog(color, input.fog);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, coverage,
                     float4(color, alpha));
}