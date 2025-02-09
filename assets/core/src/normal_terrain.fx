
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

// Disbale forced loop unroll warning.
#pragma warning(disable : 3550)

const static float4 lighting_factor = custom_constants[0];
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

   float3 static_lighting : STATICLIGHT;
   float fog : FOG;
   float2 blend_values : BLENDVALUES;
   float fade : FADE;

   float4 positionPS : SV_Position;
};

Vs_blendmap_output diffuse_blendmap_vs(Vs_input input)
{
   Vs_blendmap_output output;

   const float3 positionOS = decompress_position((float3)input.position.xyz);
   const float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   const float4 positionPS = mul(float4(positionWS, 1.0), projection_matrix);
   const float3 normalOS = input.normal.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   output.positionWS = positionWS;
   output.normalWS = mul(normalOS, (float3x3)world_matrix);
   output.positionPS = positionPS;

   [unroll] for (int i = 0; i < 4; ++i) {
      output.texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[i]);
   }

   output.blend_values.x = input.normal.w;
   output.blend_values.y = (float)input.position.w * lighting_factor.w;

   output.static_lighting = get_static_diffuse_color(input.color);
   output.fade = calculate_near_fade(positionPS);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_detail_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   noperspective float2 shadow_map_texcoords : TEXCOORD3;

   float3 static_lighting : STATICLIGHT;
   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_detail_output detailing_vs(Vs_input input)
{
   Vs_detail_output output;


   const float3 positionOS = decompress_position((float3)input.position.xyz);
   const float3 positionWS = mul(float4(positionOS, 1.0), world_matrix);
   const float4 positionPS = mul(float4(positionWS, 1.0), projection_matrix);
   const float3 normalOS = input.normal.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   output.positionWS = positionWS;
   output.normalWS = mul(normalOS, (float3x3)world_matrix);
   output.positionPS = positionPS;

   [unroll] for (int i = 0; i < 2; ++i) {
      output.detail_texcoords[i].x = dot(float4(positionWS, 1.0), x_texcoords_tranforms[i]);
      output.detail_texcoords[i].y = dot(float4(positionWS, 1.0), y_texcoords_tranforms[1]);
   }

   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_map_texcoords = (positionPS.xy / positionPS.w) * float2(0.5, -0.5) + 0.5;
   output.static_lighting = get_static_diffuse_color(input.color);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_blendmap_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 texcoords[4] : TEXCOORD0;
   
   float3 static_lighting : STATICLIGHT;
   float fog : FOG;
   float2 blend_values : BLENDVALUES;
   float fade : FADE;
};

float4 diffuse_blendmap_ps(Ps_blendmap_input input, 
                           Texture2D<float3> diffuse_maps[3] : register(t0),
                           Texture2D<float3> detail_map : register(t3)): SV_Target0
{
   const float blend_weights[3] = {(1.0 - input.blend_values.y) - input.blend_values.x,
                                   input.blend_values.x,
                                   input.blend_values.y};

   float3 diffuse_color = 0.0;

   [unroll] for (int i = 0; i < 3; ++i) {
      diffuse_color += 
         diffuse_maps[i].Sample(aniso_wrap_sampler, input.texcoords[i]) * blend_weights[i];
   }

   const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, input.texcoords[3]);

   diffuse_color = (diffuse_color * detail_color) * 2.0;

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting);

   float3 color = lighting.color * lighting_factor.x + lighting_factor.y;
   color *= diffuse_color;

   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}

struct Ps_detail_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 detail_texcoords[2] : TEXCOORD0;
   float4 projection_texcoords : TEXCOORD2;
   noperspective float2 shadow_map_texcoords : TEXCOORD3;

   float3 static_lighting : STATICLIGHT;
   float fog : FOG;
};

float4 detailing_ps(Ps_detail_input input, 
                    Texture2D<float3> detail_maps[2] : register(t0),
                    Texture2D<float3> projected_texture : register(t2),
                    Texture2D<float1> shadow_map : register(t3)) : SV_Target0
{
   const float3 detail_color = detail_maps[0].Sample(aniso_wrap_sampler, input.detail_texcoords[0]);

   const float3 projection_texture_color = sample_projected_light(projected_texture,
                                                                  input.projection_texcoords);

   // Calculate lighting.
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   float3 color = (lighting_factor.x > 0.0) ? lighting.color : lighting_scale.xxx;

   const float shadow_map_value = shadow_map.SampleLevel(linear_clamp_sampler,
                                                         input.shadow_map_texcoords, 0.0).r;
   
   const float shadow = 1.0 - (lighting.intensity * (1.0 - shadow_map_value));

   color = detail_color * shadow  * 2.0 * color;
   color = apply_fog(color, input.fog);

   return float4(color, 1.0);
}