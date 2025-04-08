#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

const static float3 specular_color = custom_constants[0].xyz;
const static float3 specular_transform_x = custom_constants[1].xyz;
const static float3 specular_transform_y = custom_constants[2].xyz;
const static float4 shield_constants = custom_constants[3];
const static float2 near_scene_fade_scale = custom_constants[4].xy;

Texture2D<float4> diffuse_texture : register(t0);
Texture2D<float3> specular_spot_texture : register(t1);

struct Vs_output {
   float2 texcoords : TEXCOORD0;
   float fog : FOG;
   float fade : FADE;
   float3 specular_texcoords : TEXCOORD1;
   float3 normalWS : NORMALWS;
   float3 positionWS : POSITIONWS;
   float4 material_color : MATCOLOR;
   float3 specular_color : SPECCOLOR;

   float4 positionPS : SV_Position;
};

float3 get_specular_texcoords(float3 reflectionWS)
{
   float3 coords;

   coords.x = dot(reflectionWS, specular_transform_x);
   coords.y = dot(reflectionWS, specular_transform_y);
   coords.z = dot(reflectionWS, -light_directional_dir(0));

   return coords + coords.z;
}

Vs_output shield_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();
   const float3 normalWS = transformer.normalWS();

   float3 viewWS = positionWS - view_positionWS;
   float VdotN = dot(-viewWS, normalWS);
   float3 reflectionWS = VdotN * normalWS + viewWS;
   reflectionWS = VdotN * normalWS + reflectionWS;

   output.positionPS = positionPS;
   output.normalWS = normalWS;
   output.positionWS = positionWS;
   output.texcoords = input.texcoords() + shield_constants.xy;
   output.specular_texcoords = get_specular_texcoords(reflectionWS);

   float fade = calculate_near_fade(positionPS);
   fade = fade * near_scene_fade_scale.x + near_scene_fade_scale.y;
   fade = fade * fade;
   fade = saturate(fade);

   output.fade = fade;
   output.fog = calculate_fog(positionWS, positionPS);
   output.material_color = get_material_color(input.color());

   float NdotL = dot(normalWS, -light_directional_dir(0));
    
   output.specular_color = (NdotL >= 0.0) && (VdotN >= 0.0) ? specular_color : 0.0;

   return output;
}


float calc_angle_alpha(float3 positionWS, float3 normalWS, float vertex_alpha)
{
   float3 V = positionWS - view_positionWS;
   float VdotN = dot(-V, normalWS);
   float3 R = VdotN * normalWS + V;
   R = VdotN * normalWS + R;

   float VdotV = max(dot(V, V), shield_constants.w); 
   float inv_VdotV = 1.0 / VdotV; 

   float RdotV = dot(R, V);

   float alpha = RdotV * inv_VdotV;
   alpha = alpha * -0.5 + 0.5;
   alpha = alpha * shield_constants.z;
   alpha = -alpha * vertex_alpha + vertex_alpha;

   return alpha;
}

struct Ps_input {
   float2 texcoords : TEXCOORD0;
   float fog : FOG;
   float fade : FADE;
   float3 specular_texcoords : TEXCOORD1;
   float3 normalWS : NORMALWS;
   float3 positionWS : POSITIONWS;
   float4 material_color : MATCOLOR;
   float3 specular_color : SPECCOLOR;
};

float4 shield_ps(Ps_input input) : SV_Target0
{
   const float angle_alpha = calc_angle_alpha(input.positionWS, normalize(input.normalWS), input.material_color.a);
   const float3 view_normalWS = normalize(input.positionWS - view_positionWS);

   const float2 texcoords = float2(input.texcoords.x, saturate(input.texcoords.y));
   const float4 diffuse_map_color = diffuse_texture.Sample(linear_wrap_sampler, texcoords);

   const float alpha = saturate(input.fade * input.material_color.a * angle_alpha);

   float3 color = diffuse_map_color.rgb * input.material_color.rgb * alpha;

   [flatten]
   if (diffuse_map_color.a + 0.49 > 0.5) {
      const float3 specular_map_color = 
         specular_spot_texture.Sample(linear_clamp_sampler, input.specular_texcoords.xy / input.specular_texcoords.z);

      color += input.specular_color * specular_map_color.rgb;
   }

   color *= lighting_scale;

   color = apply_fog(color, input.fog);

   return float4(color, diffuse_map_color.a);
}

[earlydepthstencil] 
void oit_shield_ps(Ps_input input, float4 positionSS : SV_Position, uint coverage : SV_Coverage) 
{
   float4 color = shield_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color, coverage);
}