
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"

static const float specular_exponent = 64.0;
const static uint light_count = LIGHT_COUNT;

float4 texture_transforms[2] : register(vs, c[CUSTOM_CONST_MIN]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normal : NORMAL;
   float3 tangent : BINORMAL;
   float3 bitangent : TANGENT;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
};

struct Vs_normalmapped_ouput
{
   float4 position : POSITION;

   float1 fog_eye_distance : DEPTH;
   float2 texcoords : TEXCOORD0;
   float3 world_position : TEXCOORD1;
   float3x3 TBN : TEXCOORD2;
};

Vs_normalmapped_ouput normalmapped_vs(Vs_input input)
{
   Vs_normalmapped_ouput output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);
   
   float3 world_normal = normals_to_world(transform::normals(input.normal,
                                                             input.blend_indices,
                                                             input.weights));
   float3 world_tangent = input.tangent;
   float3 world_bitangent = input.bitangent;
   transform::tangents(world_tangent, world_bitangent, input.blend_indices, input.weights);

   float3x3 TBN = {world_tangent, world_bitangent, world_normal};

   output.TBN = transpose(TBN);

   return output;
}

struct Vs_blinn_phong_ouput
{
   float4 position : POSITION;

   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 world_normal : TEXCOORD2;
};

Vs_blinn_phong_ouput blinn_phong_vs(Vs_input input)
{
   Vs_blinn_phong_ouput output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   output.world_normal = normals_to_world(transform::normals(input.normal,
                                                             input.blend_indices,
                                                             input.weights));

   return output;
}

float3 calculate_blinn_specular(float3 normal, float3 view_normal, float3 world_position,
                                float4 light_position, float3 light_color, 
                                float3 specular_color)
{
   float3 light_direction = light_position.xyz - world_position;

   float distance = length(light_direction);

   float attenuation = 1.0 - distance * distance / (1.0 / light_position.w);
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

float3 calculate_reflection(float3 world_normal, float3 view_normal)
{
   float NdotN = dot(world_normal, world_normal);
   float NdotV = dot(world_normal, view_normal);

   float3 refl = world_normal * (NdotV * 2.0);
   return -view_normal * NdotN + refl;
}

struct Ps_normalmapped_input
{
   float1 fog_eye_distance : DEPTH;
   float2 texcoords : TEXCOORD0;
   float3 world_position : TEXCOORD1;
   float3x3 TBN : TEXCOORD2;
};

float4 normalmapped_ps(Ps_normalmapped_input input,
                       uniform sampler2D normal_map,
                       uniform float4 specular_color : register(ps, c[0]),
                       uniform float3 light_color : register(ps, c[2]),
                       uniform float4 light_position : register(c[CUSTOM_CONST_MIN + 2])) : COLOR
{
   float4 normal_map_gloss = tex2D(normal_map, input.texcoords);

   float3 normal = normal_map_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   normal = normalize(mul(input.TBN, normal));

   float3 view_normal = normalize(world_view_position - input.world_position);

   float3 spec = calculate_blinn_specular(normal, view_normal, input.world_position,
                                          light_position, light_color, specular_color.rgb);

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = gloss * spec;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_gloss.a);
}

float4 normalmapped_envmap_ps(Ps_normalmapped_input input,
                              uniform sampler2D normal_map, 
                              uniform samplerCUBE envmap : register(ps, s[3]),
                              uniform float4 specular_color : register(ps, c[0]),
                              uniform float3 light_color : register(ps, c[2])) : COLOR
{
   float4 normal_map_gloss = tex2D(normal_map, input.texcoords);

   float3 normal = normal_map_gloss.xyz * 255.0 / 127.0 - 128.0 / 127.0;

   normal = normalize(mul(input.TBN, normal));

   float3 view_normal = normalize(world_view_position - input.world_position);
   float3 envmap_color = texCUBE(envmap, calculate_reflection(normal, view_normal)).rgb;

   float gloss = lerp(1.0, normal_map_gloss.a, specular_color.a);
   float3 color = light_color * envmap_color * gloss;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_gloss.a);
}

struct Ps_blinn_phong_input
{
   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 world_normal : TEXCOORD2;
};

float4 blinn_phong_ps(Ps_blinn_phong_input input,
                      uniform sampler2D diffuse_map, uniform samplerCUBE envmap,
                      uniform float4 specular_color : register(ps, c[0]),
                      uniform float3 light_colors[3] : register(ps, c[2]),
                      uniform float envmap_state : register(c[CUSTOM_CONST_MIN + 2]),
                      uniform float4 light_positions[3] : register(c[CUSTOM_CONST_MIN + 3])) : COLOR
{
   float alpha = tex2D(diffuse_map, input.texcoords).a;
   float gloss = lerp(1.0, alpha, specular_color.a);

   float3 normal = normalize(input.world_normal);
   float3 view_normal = normalize(world_view_position - input.world_position);

   float3 color = float3(0.0, 0.0, 0.0);

   if (light_count >= 1) {
      float3 spec_contrib = calculate_blinn_specular(normal, view_normal,
                                                     input.world_position,
                                                     light_positions[0], light_colors[0], 
                                                     specular_color.rgb);
      const float3 env_color = 
         texCUBE(envmap, calculate_reflection(normal, view_normal)).rgb * specular_color.rgb;

      color += lerp(spec_contrib, env_color, envmap_state);
   }
   
   [unroll] for (uint i = 1; i < light_count; ++i) {
      color += calculate_blinn_specular(normal, view_normal, input.world_position,
                                        light_positions[i], light_colors[i], 
                                        specular_color.rgb);
   }

   color *= gloss;
   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, alpha);
}

float4 debug_vertexlit_ps() : COLOR
{
   return float4(1.0, 1.0, 0.0, 1.0);
}

