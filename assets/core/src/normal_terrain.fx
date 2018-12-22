
#include "ext_constants_list.hlsl"
#include "vertex_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "pixel_sampler_states.hlsl"

// Disbale forced loop unroll warning.
#pragma warning(disable : 3550)

const static float4 lighting_constant = custom_constants[0];
const static float4 x_texcoords_tranforms[4] = {custom_constants[1], custom_constants[3],
                                                custom_constants[5], custom_constants[7]};
const static float4 y_texcoords_tranforms[4] = {custom_constants[2], custom_constants[4],
                                                custom_constants[6], custom_constants[8]};

struct Vs_input {
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
   unorm float4 color : COLOR;
};

struct Vs_blendmap_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 texcoords[4] : TEXCOORD0;

   float3 blend_values_fade : BLENDVALUES;
   float3 static_lighting : STATICLIGHT;
   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_blendmap_output diffuse_blendmap_vs(Vs_input input)
{
   Vs_blendmap_output output;

   const float3 positionOS = decompress_position((float3)input.position.xyz);
   const float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   const float3 normalOS = input.normal.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   output.positionWS = positionWS;
   output.normalWS = mul(normalOS, (float3x3)world_matrix);
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   [unroll] for (int i = 0; i < 4; ++i) {
      output.texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[i]);
   }

   float near_fade, fog;
   calculate_near_fade_and_fog(positionWS, near_fade, fog);

   output.blend_values_fade.x = input.normal.w;
   output.blend_values_fade.y = (float)input.position.w * lighting_constant.w; 
   output.blend_values_fade.z = saturate(near_fade);

   output.static_lighting = get_static_diffuse_color(input.color);
   output.fog = fog;

   return output;
}

struct Vs_detail_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_map_texcoords : TEXCOORD3;

   float3 static_lighting : STATICLIGHT;
   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_detail_output detailing_vs(Vs_input input)
{
   Vs_detail_output output;


   const float3 positionOS = decompress_position((float3)input.position.xyz);
   const float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   const float3 normalOS = input.normal.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   output.positionWS = positionWS;
   output.normalWS = mul(normalOS, (float3x3)world_matrix);
   output.positionPS = mul(float4(positionWS, 1.0), projection_matrix);

   [unroll] for (int i = 0; i < 2; ++i) {
      output.detail_texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.detail_texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[1]);
   }

   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_map_texcoords = transform_shadowmap_coords(positionWS);
   output.static_lighting = get_static_diffuse_color(input.color);

   float near_fade, fog;
   calculate_near_fade_and_fog(positionWS, near_fade, fog);

   output.fog = fog;

   return output;
}

struct Ps_blendmap_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 texcoords[4] : TEXCOORD0;

   float3 blend_values_fade : BLENDVALUES;
   float3 static_lighting : STATICLIGHT;
   float fog : FOG;
};

float4 diffuse_blendmap_ps(Ps_blendmap_input input, 
                           Texture2D<float3> diffuse_maps[3] : register(t0),
                           Texture2D<float3> detail_map : register(t3)): SV_Target0
{
   const float blend_weights[3] = {(1.0 - input.blend_values_fade.y) - input.blend_values_fade.x,
                                   input.blend_values_fade.x,
                                   input.blend_values_fade.y};

   float3 diffuse_color = 0.0;

   [unroll] for (int i = 0; i < 3; ++i) {
      diffuse_color += 
         diffuse_maps[i].Sample(aniso_wrap_sampler, input.texcoords[i]) * blend_weights[i];
   }

   const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, input.texcoords[3]);

   diffuse_color = (diffuse_color * detail_color) * 2.0;

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting);

   lighting.color = lighting.color * lighting_constant.x + lighting_constant.y;

   float3 color = diffuse_color * lighting.color;

   // Linear Rendering Normalmap Hack
   color = lerp(color, diffuse_color * lighting_scale, rt_multiply_blending_state);
   color = apply_fog(color, input.fog);

   return float4(color, input.blend_values_fade.z);
}

struct Ps_detail_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_map_texcoords : TEXCOORD3;

   float3 static_lighting : STATICLIGHT;
   float fog : FOG;
};

float4 detailing_ps(Ps_detail_input input, 
                    Texture2D<float3> detail_maps[2] : register(t0),
                    Texture2D<float3> projected_texture : register(t2),
                    Texture2D<float4> shadow_map : register(t3)) : SV_Target0
{
   const float3 detail_color_0 = detail_maps[0].Sample(aniso_wrap_sampler, input.detail_texcoords[0]);
   const float3 detail_color_1 = detail_maps[1].Sample(aniso_wrap_sampler, input.detail_texcoords[1]);

   const float3 projection_texture_color = sample_projected_light(projected_texture,
                                                                  input.projection_texcoords);

   // Calculate lighting.
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   lighting.color = lighting.color * lighting_constant.x + lighting_constant.y;

   float3 color = lighting.color;

   const float2 shadow_texcoords = input.shadow_map_texcoords.xy / input.shadow_map_texcoords.w;
   const float shadow_map_value = shadow_map.SampleLevel(linear_clamp_sampler,
                                                         shadow_texcoords, 0.0).a;
   
   const float shadow = 1.0 - (lighting.intensity * (1.0 - shadow_map_value));
   
   float3 blended_detail_color = (detail_color_0 * shadow)  * 2.0;
   blended_detail_color *=  (detail_color_1 * 2.0);

   color *= blended_detail_color;

   // Linear Rendering Normalmap Hack
   color = lerp(color, blended_detail_color, rt_multiply_blending_state);
   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}