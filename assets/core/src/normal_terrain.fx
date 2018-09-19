
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

// Disbale forced loop unroll warning.
#pragma warning(disable : 3550)

float4 lighting_constant : register(c[CUSTOM_CONST_MIN]);
float4 texcoords_transforms[8] : register(vs, c[CUSTOM_CONST_MIN + 1]);

const static float4 x_texcoords_tranforms[4] = {texcoords_transforms[0], texcoords_transforms[2], 
                                                texcoords_transforms[4], texcoords_transforms[6]};

const static float4 y_texcoords_tranforms[4] = {texcoords_transforms[1], texcoords_transforms[3], 
                                                texcoords_transforms[5], texcoords_transforms[7]};

SamplerState linear_wrap_sampler;

struct Vs_input {
   int4 position : POSITION;
   unorm float4 normal : NORMAL;
   unorm float4 color : COLOR;
};

struct Vs_blendmap_output
{
   float4 positionPS : SV_Position;
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords[4] : TEXCOORD2;
   float3 blend_values_fade : TEXCOORD6;
   float3 static_lighting : TEXCOORD7;
   float fog_eye_distance : DEPTH;
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
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   [unroll] for (int i = 0; i < 4; ++i) {
      output.texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[i]);
   }

   Near_scene near_scene = calculate_near_scene_fade(positionWS);

   output.blend_values_fade.x = input.normal.w;
   output.blend_values_fade.y = (float)input.position.w * lighting_constant.w; 
   output.blend_values_fade.z = saturate(near_scene.fade);

   output.static_lighting = get_static_diffuse_color(input.color);

   return output;
}

struct Vs_detail_output
{
   float4 positionPS : SV_Position;
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;

   float2 detail_texcoords[2] : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD4;
   float4 shadow_map_texcoords : TEXCOORD5;
   float3 static_lighting : TEXCOORD6;

   float fog_eye_distance : DEPTH;
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
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   [unroll] for (int i = 0; i < 2; ++i) {
      output.detail_texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.detail_texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[1]);
   }

   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_map_texcoords = transform_shadowmap_coords(positionWS);
   output.static_lighting = get_static_diffuse_color(input.color);

   return output;
}

struct Ps_blendmap_input
{
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords[4] : TEXCOORD2;
   float3 blend_values_fade : TEXCOORD6;
   float3 static_lighting : TEXCOORD7;
   float fog_eye_distance : DEPTH;
};

float4 diffuse_blendmap_ps(Ps_blendmap_input input, 
                           Texture2D<float3> diffuse_maps[3] : register(ps_3_0, s0),
                           Texture2D<float3> detail_map : register(ps_3_0, s3)): SV_Target0
{
   const float blend_weights[3] = {(1.0 - input.blend_values_fade.y) - input.blend_values_fade.x,
                                   input.blend_values_fade.x,
                                   input.blend_values_fade.y};

   float3 diffuse_color = 0.0;

   [unroll] for (int i = 0; i < 3; ++i) {
      diffuse_color += 
         diffuse_maps[i].Sample(linear_wrap_sampler, input.texcoords[i]) * blend_weights[i];
   }

   const float3 detail_color = detail_map.Sample(linear_wrap_sampler, input.texcoords[3]);

   diffuse_color = (diffuse_color * detail_color) * 2.0;

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting);

   lighting.color = lighting.color * lighting_constant.x + lighting_constant.y;

   float3 color = diffuse_color * lighting.color;

   // Linear Rendering Normalmap Hack
   color = lerp(color, diffuse_color * hdr_info.z, rt_multiply_blending);
   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, input.blend_values_fade.z);
}

struct Ps_detail_input
{
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;

   float2 detail_texcoords[2] : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD4;
   float4 shadow_map_texcoords : TEXCOORD5;
   float3 static_lighting : TEXCOORD6;

   float fog_eye_distance : DEPTH;
};

float4 detailing_ps(Ps_detail_input input, 
                    Texture2D<float3> detail_maps[2] : register(ps_3_0, s0),
                    Texture2D<float3> projected_texture : register(ps_3_0, s2),
                    Texture2D<float4> shadow_map : register(ps_3_0, s3)) : SV_Target0
{
   const float3 detail_color_0 = detail_maps[0].Sample(linear_wrap_sampler, input.detail_texcoords[0]);
   const float3 detail_color_1 = detail_maps[1].Sample(linear_wrap_sampler, input.detail_texcoords[1]);

   const float3 projection_texture_color = sample_projected_light(projected_texture,
                                                                  input.projection_texcoords);

   // Calculate lighting.
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   lighting.color = lighting.color * lighting_constant.x + lighting_constant.y;

   float3 color = lighting.color;

   const float2 shadow_texcoords = input.shadow_map_texcoords.xy / input.shadow_map_texcoords.w;
   const float shadow_map_value = shadow_map.SampleLevel(linear_wrap_sampler, 
                                                         shadow_texcoords, 0.0).a;
   
   const float shadow = 1.0 - (lighting.intensity * (1.0 - shadow_map_value));
   
   float3 blended_detail_color = (detail_color_0 * shadow)  * 2.0;
   blended_detail_color *=  (detail_color_1 * 2.0);

   color *= blended_detail_color;

   // Linear Rendering Normalmap Hack
   color = lerp(color, blended_detail_color, rt_multiply_blending);
   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}