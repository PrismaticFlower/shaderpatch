
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"

const static float4 specular_color = ps_custom_constants[0];
static const float specular_exponent = 64.0;
const static uint light_count = SPECULAR_LIGHT_COUNT;
const static float3 light_colors[3] = {ps_custom_constants[2].xyz,
                                       ps_custom_constants[3].xyz,
                                       ps_custom_constants[4].xyz};

const static float4 x_texcoords_transform = custom_constants[0];
const static float4 y_texcoords_transform = custom_constants[1];

struct Vs_normalmapped_ouput {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD1;
   float3x3 TBN : TBN;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_normalmapped_ouput normalmapped_vs(Vertex_input input)
{
   Vs_normalmapped_ouput output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;

   output.texcoords =
      transformer.texcoords(x_texcoords_transform, y_texcoords_transform);

   const float3 normalWS = transformer.normalWS();
   const float3 tangentWS = transformer.tangentWS();
   const float3 bitangentWS = transformer.bitangentWS();

   const float3x3 TBN = {tangentWS, bitangentWS, normalWS};

   output.TBN = transpose(TBN);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_blinn_phong_ouput {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float2 texcoords : TEXCOORD;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_blinn_phong_ouput blinn_phong_vs(Vertex_input input)
{
   Vs_blinn_phong_ouput output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();

   output.texcoords =
      transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

float3 calculate_blinn_specular(float3 N, float3 V, float3 position,
                                float4 light_position, float3 light_color)
{
   float3 L = normalize(light_position.xyz - position);

   const float dst = distance(light_position.xyz, position);
   const float inv_range_sq = light_position.w;
   float attenuation = saturate(1.0 - ((dst * dst) * inv_range_sq));

   if (light_position.w == 0) {
      L = light_position.xyz;
      attenuation = 1.0;
   }

   const float3 H = normalize(L + V);
   const float NdotH = saturate(dot(N, H));
   const float specular = pow(NdotH, specular_exponent);

   return attenuation * light_color * specular;
}

struct Ps_normalmapped_input {
   float3 positionWS : POSITIONWS;
   float2 texcoords : TEXCOORD1;
   float3x3 TBN : TBN;

   float fog : FOG;
};

float4 normalmapped_ps(Ps_normalmapped_input input, uniform Texture2D<float4> normal_map
                       : register(t0))
   : SV_Target0
{
   const float4 light_positionWS = custom_constants[2];

   float4 normal_map_gloss = normal_map.Sample(aniso_wrap_sampler, input.texcoords);

   float3 normalTS = normal_map_gloss.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 view_normalWS = normalize(view_positionWS - input.positionWS);

   float3 spec = calculate_blinn_specular(normalWS, view_normalWS, input.positionWS,
                                          light_positionWS, light_colors[0]);

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = gloss * specular_color.rgb * spec;

   color = apply_fog(color, input.fog);

   return float4(color, normal_map_gloss.a);
}

float4 normalmapped_envmap_ps(Ps_normalmapped_input input, uniform Texture2D<float4> normal_map
                              : register(t0), uniform TextureCube<float3> envmap
                              : register(t3))
   : SV_Target0
{
   float4 normal_map_gloss = normal_map.Sample(aniso_wrap_sampler, input.texcoords);

   float3 normalTS = normal_map_gloss.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 view_normalWS = normalize(view_positionWS - input.positionWS);
   float3 reflectionWS = calculate_envmap_reflection(normalWS, view_normalWS);

   float3 envmap_color = envmap.Sample(aniso_wrap_sampler, reflectionWS);

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = light_colors[0] * envmap_color * gloss;

   color = apply_fog(color, input.fog);

   return float4(color, normal_map_gloss.a);
}

struct Ps_blinn_phong_input {
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;
   float2 texcoords : TEXCOORD;

   float fog : FOG;
};

float4 blinn_phong_ps(Ps_blinn_phong_input input, uniform Texture2D<float4> diffuse_map
                      : register(t0), uniform TextureCube<float3> envmap
                      : register(t1))
   : SV_Target0
{
   const float envmap_state = custom_constants[2].x;
   const float4 light_positionsWS[3] = {custom_constants[3], custom_constants[4],
                                        custom_constants[5]};

   float alpha = diffuse_map.Sample(aniso_wrap_sampler, input.texcoords).a;
   float gloss = lerp(1.0, alpha, specular_color.a);

   float3 normalWS = normalize(input.normalWS);
   float3 view_normalWS = normalize(view_positionWS - input.positionWS);

   float3 color = float3(0.0, 0.0, 0.0);

   if (light_count >= 1) {
      float3 spec_contrib =
         calculate_blinn_specular(normalWS, view_normalWS, input.positionWS,
                                  light_positionsWS[0], light_colors[0]);

      float3 reflectionWS = calculate_envmap_reflection(normalWS, view_normalWS);

      float3 env_color = envmap.Sample(aniso_wrap_sampler, reflectionWS);

      color += lerp(spec_contrib, env_color, envmap_state);
   }

   [unroll] for (uint i = 1; i < light_count; ++i)
   {
      color += calculate_blinn_specular(normalWS, view_normalWS, input.positionWS,
                                        light_positionsWS[i], light_colors[i]);
   }

   color *= (specular_color.rgb * gloss);
   color = apply_fog(color, input.fog);

   return float4(color, alpha);
}

float4 debug_vertexlit_ps() : SV_Target0
{
   return float4(1.0, 1.0, 0.0, 1.0);
}
