
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"

static const float specular_exponent = 64.0;
const static uint light_count = SPECULAR_LIGHT_COUNT;

float4 x_texcoords_transform : register(vs, c[CUSTOM_CONST_MIN]);
float4 y_texcoords_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);

SamplerState linear_wrap_sampler;

struct Vs_normalmapped_ouput
{
   float4 positionPS : SV_Position;

   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3x3 TBN : TEXCOORD2;

   float1 fog_eye_distance : DEPTH;
};

Vs_normalmapped_ouput normalmapped_vs(Vertex_input input)
{
   Vs_normalmapped_ouput output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.texcoords = transformer.texcoords(x_texcoords_transform,
                                            y_texcoords_transform);
   
   float3 normalWS = transformer.normalWS();
   float3 tangentWS = transformer.tangentWS();
   float3 bitangent = transformer.bitangentWS();

   float3x3 TBN = {tangentWS, bitangent, normalWS};

   output.TBN = transpose(TBN);

   return output;
}

struct Vs_blinn_phong_ouput
{
   float4 positionPS : SV_Position;

   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;

   float1 fog_eye_distance : DEPTH;
};

Vs_blinn_phong_ouput blinn_phong_vs(Vertex_input input)
{
   Vs_blinn_phong_ouput output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();

   output.texcoords = transformer.texcoords(x_texcoords_transform,
                                            y_texcoords_transform);

   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   return output;
}

float3 calculate_blinn_specular(float3 normal, float3 view_normal, float3 world_position,
                                float4 light_position, float3 light_color, 
                                float3 specular_color)
{
   float3 light_direction = light_position.xyz - world_position;

   float distance = length(light_direction);

   const float inv_range_sq = light_position.w;

   float attenuation = 1.0 - (distance * distance) * inv_range_sq;
   attenuation = saturate(attenuation);
   attenuation *= attenuation;

   if (light_position.w == 0) {
      light_direction = light_position.xyz;
      attenuation = 1.0;
   }
   
   light_direction = normalize(light_direction);

   const float3 H = normalize(light_direction + view_normal);
   const float NdotH = saturate(dot(normal, H));
   const float specular = pow(NdotH, specular_exponent);

   return attenuation * (specular_color * light_color * specular);
}

float3 calculate_reflection(float3 normal, float3 view_normal)
{
   float NdotN = dot(normal, normal);
   float NdotV = dot(normal, view_normal);

   float3 refl = normal * (NdotV * 2.0);
   return -view_normal * NdotN + refl;
}

struct Ps_normalmapped_input
{
   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3x3 TBN : TEXCOORD2;

   float1 fog_eye_distance : DEPTH;
};

float4 normalmapped_ps(Ps_normalmapped_input input,
                       uniform Texture2D<float4> normal_map : register(ps_3_0, s0),
                       uniform float4 specular_color : register(ps, c[0]),
                       uniform float3 light_color : register(ps, c[2]),
                       uniform float4 light_positionWS : register(c[CUSTOM_CONST_MIN + 2])) : SV_Target0
{
   float4 normal_map_gloss = normal_map.Sample(linear_wrap_sampler, input.texcoords);

   float3 normalTS = normal_map_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 view_normalWS = normalize(view_positionWS - input.positionWS);

   float3 spec = calculate_blinn_specular(normalWS, view_normalWS, input.positionWS,
                                          light_positionWS, light_color, specular_color.rgb);

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = gloss * spec;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_gloss.a);
}

float4 normalmapped_envmap_ps(Ps_normalmapped_input input,
                              uniform Texture2D<float4> normal_map : register(ps_3_0, s0),
                              uniform TextureCube<float3> envmap : register(ps_3_0, s3),
                              uniform float4 specular_color : register(ps, c0),
                              uniform float3 light_color : register(ps, c2)) : SV_Target0
{
   float4 normal_map_gloss = normal_map.Sample(linear_wrap_sampler, input.texcoords);

   float3 normalTS = normal_map_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 view_normalWS = normalize(view_positionWS - input.positionWS);
   float3 reflectionWS = calculate_reflection(normalWS, view_normalWS);

   float3 envmap_color = envmap.Sample(linear_wrap_sampler, reflectionWS);

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = light_color * envmap_color * gloss;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_gloss.a);
}

struct Ps_blinn_phong_input
{
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;

   float1 fog_eye_distance : DEPTH;
};

float4 blinn_phong_ps(Ps_blinn_phong_input input,
                      uniform Texture2D<float4> diffuse_map : register(ps_3_0, s0),
                      uniform TextureCube<float3> envmap : register(ps_3_0, s1),
                      uniform float4 specular_color : register(ps, c[0]),
                      uniform float3 light_colors[3] : register(ps, c[2]),
                      uniform float envmap_state : register(c[CUSTOM_CONST_MIN + 2]),
                      uniform float4 light_positionsWS[3] : register(c[CUSTOM_CONST_MIN + 3])) : SV_Target0
{
   float alpha = diffuse_map.Sample(linear_wrap_sampler, input.texcoords).a;
   float gloss = lerp(1.0, alpha, specular_color.a);

   float3 normalWS = normalize(input.normalWS);
   float3 view_normalWS = normalize(view_positionWS - input.positionWS);

   float3 color = float3(0.0, 0.0, 0.0);

   if (light_count >= 1) {
      float3 spec_contrib = calculate_blinn_specular(normalWS, view_normalWS,
                                                     input.positionWS,
                                                     light_positionsWS[0], light_colors[0], 
                                                     specular_color.rgb);
      
      float3 reflectionWS = calculate_reflection(normalWS, view_normalWS);
      
      float3 env_color = envmap.Sample(linear_wrap_sampler, reflectionWS) * specular_color.rgb;

      color += lerp(spec_contrib, env_color, envmap_state);
   }
   
   [unroll] for (uint i = 1; i < light_count; ++i) {
      color += calculate_blinn_specular(normalWS, view_normalWS, input.positionWS,
                                        light_positionsWS[i], light_colors[i], 
                                        specular_color.rgb);
   }

   color *= gloss;
   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, alpha);
}

float4 debug_vertexlit_ps() : SV_Target0
{
   return float4(1.0, 1.0, 0.0, 1.0);
}

