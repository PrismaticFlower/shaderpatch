
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

float4 x_texcoords_transform : register(vs, c[CUSTOM_CONST_MIN]);
float4 y_texcoords_transform : register(vs, c[CUSTOM_CONST_MIN + 1]);

float4 light_constants[7] : register(c[21]);

float3 light_colors[3] : register(ps, c[0]);

const static float4 light_positionsWS[3] = { light_constants[0], light_constants[2], light_constants[4] };
const static float light_inv_radiuses_sq[3] = {light_constants[1].x, light_constants[3].x, light_constants[5].x};

const static float3 spotlight_color = light_colors[0];
const static float3 spotlight_positionWS =  light_constants[0].xyz;
const static float spotlight_inv_radiuses_sq = light_constants[1].x;
const static float3 spotlight_directionWS = light_constants[2].xyz;

const static bool generate_texcoords = PERPIXEL_GENERATE_TEXCOORDS;
const static bool generate_tangents = PERPIXEL_GENERATE_TANGENTS;
const static uint light_count = PERPIXEL_LIGHT_COUNT;

SamplerState linear_wrap_sampler;

struct Vs_perpixel_output
{
   float4 positionPS : SV_Position;

   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
   float3 static_lighting : TEXCOORD3;

   float1 fog_eye_distance : DEPTH;
};

Vs_perpixel_output perpixel_vs(Vertex_input input)
{
   Vs_perpixel_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.texcoords = transformer.texcoords(x_texcoords_transform,
                                            y_texcoords_transform);

   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

struct Vs_normalmapped_output
{
   float4 positionPS : SV_Position;

   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;

   float3x3 TBN : TEXCOORD3;

   float1 fog_eye_distance : DEPTH;
};

Vs_normalmapped_output normalmapped_vs(Vertex_input input)
{
   Vs_normalmapped_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(positionWS, 1.0), x_texcoords_transform.xzyw);
      output.texcoords.y = dot(float4(positionWS, 1.0), y_texcoords_transform.xzyw);
   }
   else {
      output.texcoords = transformer.texcoords(x_texcoords_transform,
                                               y_texcoords_transform);
   }

   float3 normalWS = transformer.normalWS();
   float3 tangentWS; 
   float3 bitangentWS;

   if (generate_tangents) {
      generate_terrain_tangents(normalWS, tangentWS, bitangentWS);
   }
   else {
      tangentWS = transformer.tangentWS();
      bitangentWS = transformer.bitangentWS();
   }

   float3x3 TBN = {tangentWS, bitangentWS, normalWS};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

float4 transform_spotlight_projection(float3 positionWS)
{
   float4 projection_coords;

   projection_coords.x = dot(float4(positionWS, 1.0), light_constants[3]);
   projection_coords.y = dot(float4(positionWS, 1.0), light_constants[4]);
   projection_coords.z = dot(float4(positionWS, 1.0), light_constants[5]);
   projection_coords.w = dot(float4(positionWS, 1.0), light_constants[6]);

   return projection_coords;
}

struct Vs_perpixel_spotlight_output
{
   float4 positionPS : SV_Position;

   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
   float4 projection_coords : TEXCOORD3;
   float3 static_lighting : TEXCOORD4;

   float1 fog_eye_distance : DEPTH;
};

Vs_perpixel_spotlight_output perpixel_spotlight_vs(Vertex_input input)
{
   Vs_perpixel_spotlight_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   output.texcoords = transformer.texcoords(x_texcoords_transform,
                                            y_texcoords_transform);

   output.projection_coords = transform_spotlight_projection(positionWS);
   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

struct Vs_normalmapped_spotlight_output
{
   float4 positionPS : SV_Position;
   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float4 projection_coords : TEXCOORD2;
   float3 static_lighting : TEXCOORD3;

   float3x3 TBN : TEXCOORD4;

   float1 fog_eye_distance : DEPTH;
};

Vs_normalmapped_spotlight_output normalmapped_spotlight_vs(Vertex_input input)
{
   Vs_normalmapped_spotlight_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(positionWS, 1.0), x_texcoords_transform.xzyw);
      output.texcoords.y = dot(float4(positionWS, 1.0), y_texcoords_transform.xzyw);
   }
   else {
      output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);
   }

   output.projection_coords = transform_spotlight_projection(positionWS);

   float3 normalWS = transformer.normalWS();
   float3 tangentWS;
   float3 bitangentWS;

   if (generate_tangents) {
      generate_terrain_tangents(normalWS, tangentWS, bitangentWS);
   }
   else {
      tangentWS = transformer.tangentWS();
      bitangentWS = transformer.bitangentWS();
   }

   float3x3 TBN = {tangentWS, bitangentWS, normalWS};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

float3 calculate_light(float3 position, float3 normal, float4 light_position,
                       float3 light_color, float light_inv_radius_sq)
{
   float3 light_direction;
   float attenuation;

   [branch] if (light_position.w > 0) {
      light_direction = light_position.xyz - position;

      float distance = length(light_direction);

      light_direction = normalize(light_direction);

      attenuation = 1.0 - (distance * distance) * light_inv_radius_sq;
      attenuation = saturate(attenuation);
      attenuation *= attenuation;
   }
   else {
      light_direction = -light_position.xyz;
      attenuation = 1.0;
   }

   float intensity = max(dot(normal, light_direction), 0.0);
   
   return attenuation * (intensity * light_color);
}

struct Ps_normalmapped_input
{
   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;

   float3x3 TBN : TEXCOORD3;

   float1 fog_eye_distance : DEPTH;
};

float4 normalmapped_ps(Ps_normalmapped_input input,
                       Texture2D<float3> normal_map : register(ps_3_0, s0)) : SV_Target0
{
   const float3 normalTS = 
      normal_map.Sample(linear_wrap_sampler, input.texcoords).rgb * 255.0 / 127.0 - 128.0 / 127.0;

   const float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;

   [unroll] for (uint i = 0; i < light_count; ++i) {
      color += calculate_light(input.positionWS, normalWS,
                               light_positionsWS[i], light_colors[i], 
                               light_inv_radiuses_sq[i]);
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

struct Ps_perpixel_input
{
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
   float3 static_lighting : TEXCOORD3;

   float1 fog_eye_distance : DEPTH;
};

float4 perpixel_ps(Ps_perpixel_input input) : SV_Target0
{
   const float3 normalWS = normalize(input.normalWS);

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;

   [unroll] for (uint i = 0; i < light_count; ++i) {
      color += calculate_light(input.positionWS, normalWS,
                               light_positionsWS[i], light_colors[i], 
                               light_inv_radiuses_sq[i]);
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float3 calculate_spotlight(float3 world_position, float3 world_normal, 
                           float3 light_position, float3 light_direction, 
                           float3 light_color, float light_inv_radius_sq,
                           float3 projection_color)
{
   float3 light_normal = normalize(light_position - world_position);
   float intensity = max(dot(world_normal, light_normal), 0.0);

   float light_distance = distance(world_position, light_position);

   float attenuation = dot(light_direction, -light_normal);
   attenuation -= light_distance * light_distance * light_inv_radius_sq;
   attenuation = saturate(attenuation);

   return attenuation * (intensity * light_color) * projection_color;
}

struct Ps_perpixel_spotlight_input
{
   float3 positionWS : TEXCOORD0;
   float3 normalWS : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
   float4 projection_coords : TEXCOORD3;
   float3 static_lighting : TEXCOORD4;

   float1 fog_eye_distance : DEPTH;
};

float4 perpixel_spotlight_ps(Ps_perpixel_spotlight_input input,
                             Texture2D<float3> projection_map : register(ps_3_0, s2)) : SV_Target0
{
   const float3 projection_color = sample_projected_light(projection_map,
                                                          input.projection_coords);

   const float3 normalWS = normalize(input.normalWS);

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;

   color += calculate_spotlight(input.positionWS, normalWS, spotlight_positionWS,
                                spotlight_directionWS, spotlight_color,
                                spotlight_inv_radiuses_sq, projection_color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

struct Ps_normalmapped_spotlight_input
{
   float3 positionWS : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float4 projection_coords : TEXCOORD2;
   float3 static_lighting : TEXCOORD3;

   float3x3 TBN : TEXCOORD4;

   float1 fog_eye_distance : DEPTH;
};

float4 normalmapped_spotlight_ps(Ps_normalmapped_spotlight_input input,
                                 Texture2D<float3> normal_map : register(ps_3_0, s0),
                                 Texture2D<float3> projection_map : register(ps_3_0, s2)) : SV_Target0
{
   const float3 projection_color = sample_projected_light(projection_map,
                                                          input.projection_coords);

   const float3 normalTS =
      normal_map.Sample(linear_wrap_sampler, input.texcoords) * 255.0 / 127.0 - 128.0 / 127.0;

   const float3 normalWS = normalize(mul(input.TBN, normalTS));

   float3 color = input.static_lighting;
   color += light::ambient(normalWS);
   color *= light_ambient_color_top.a;

   color += calculate_spotlight(input.positionWS, normalWS, spotlight_positionWS,
                                spotlight_directionWS, spotlight_color,
                                spotlight_inv_radiuses_sq, projection_color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}
