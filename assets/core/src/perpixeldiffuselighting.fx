
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

float4 texture_transforms[2] : register(vs, c[CUSTOM_CONST_MIN]);

float4 light_constants[7] : register(c[21]);

float3 light_colors[3] : register(ps, c[0]);

const static float4 light_positions[3] = { light_constants[0], light_constants[2], light_constants[4] };
const static float4 light_params[3] = { light_constants[1], light_constants[3], light_constants[5] };

const static float3 spotlight_position =  light_constants[0].xyz;
const static float4 spotlight_params = light_constants[1];
const static float3 spotlight_direction = light_constants[2].xyz;

const static bool generate_texcoords = GENERATE_TEXCOORDS;
const static bool generate_tangents = GENERATE_TANGENTS;
const static uint light_count = LIGHT_COUNT;

float get_light_radius(float4 light_params)
{
   return (1.0 / light_params.x) * light_params.y;
}

struct Vs_input
{
   float4 position : POSITION;
   float3 normal : NORMAL;
   float3 tangent : BINORMAL;
   float3 bitangent : TANGENT;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_perpixel_output
{
   float4 position : POSITION;
   float1 fog_eye_distance : DEPTH;

   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float3 world_normal : TEXCOORD3;
};

Vs_perpixel_output perpixel_vs(Vs_input input)
{
   Vs_perpixel_output output;

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

   output.static_lighting = get_static_diffuse_color(input.color);

   return output;
}

struct Vs_normalmapped_output
{
   float4 position : POSITION;
   float1 fog_eye_distance : DEPTH;

   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;

   float3x3 TBN : TEXCOORD3;
};

Vs_normalmapped_output normalmapped_vs(Vs_input input)
{
   Vs_normalmapped_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(output.world_position, 1.0), texture_transforms[0].xzyw);
      output.texcoords.y = dot(float4(output.world_position, 1.0), texture_transforms[1].xzyw);
   }
   else {
      output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                        texture_transforms[0],
                                                        texture_transforms[1]);
   }

   float3 world_normal = normals_to_world(transform::normals(input.normal,
                                                             input.blend_indices,
                                                             input.weights));

   float3 world_tangent;
   float3 world_bitangent;

   if (generate_tangents) {
      generate_terrain_tangents(world_normal, world_tangent, world_bitangent);
   }
   else {
      world_tangent = input.tangent;
      world_bitangent = input.bitangent;
      transform::tangents(world_tangent, world_bitangent, input.blend_indices, input.weights);
   }

   float3x3 TBN = {world_tangent, world_bitangent, world_normal};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color);

   return output;
}

float4 transform_spotlight_projection(float4 world_position)
{
   float4 projection_coords;

   projection_coords.x = dot(world_position, light_constants[3]);
   projection_coords.y = dot(world_position, light_constants[4]);
   projection_coords.z = dot(world_position, light_constants[5]);
   projection_coords.w = dot(world_position, light_constants[6]);

   return projection_coords;
}

struct Vs_perpixel_spotlight_output
{
   float4 position : POSITION;

   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float3 world_normal : TEXCOORD3;
   float4 projection_coords : TEXCOORD4;
};

Vs_perpixel_spotlight_output perpixel_spotlight_vs(Vs_input input)
{
   Vs_perpixel_spotlight_output output;

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

   output.static_lighting = get_static_diffuse_color(input.color);
   output.projection_coords = transform_spotlight_projection(world_position);

   return output;
}

struct Vs_normalmapped_spotlight_output
{
   float4 position : POSITION;
   float1 fog_eye_distance : DEPTH;

   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float4 projection_coords : TEXCOORD3;

   float3x3 TBN : TEXCOORD4;
};

Vs_normalmapped_spotlight_output normalmapped_spotlight_vs(Vs_input input)
{
   Vs_normalmapped_spotlight_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   if (generate_texcoords) {
      output.texcoords.x = dot(float4(output.world_position, 1.0), texture_transforms[0].xzyw);
      output.texcoords.y = dot(float4(output.world_position, 1.0), texture_transforms[1].xzyw);
   }
   else {
      output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                        texture_transforms[0],
                                                        texture_transforms[1]);
   }

   float3 world_normal = normals_to_world(transform::normals(input.normal,
                                                             input.blend_indices,
                                                             input.weights));

   float3 world_tangent;
   float3 world_bitangent;

   if (generate_tangents) {
      generate_terrain_tangents(world_normal, world_tangent, world_bitangent);
   }
   else {
      world_tangent = input.tangent;
      world_bitangent = input.bitangent;
      transform::tangents(world_tangent, world_bitangent, input.blend_indices, input.weights);
   }

   float3x3 TBN = {world_tangent, world_bitangent, world_normal};

   output.TBN = transpose(TBN);

   output.static_lighting = get_static_diffuse_color(input.color);
   output.projection_coords = transform_spotlight_projection(world_position);

   return output;
}

float3 calculate_light(float3 world_position, float3 world_normal, float4 light_position,
                       float3 light_color, float radius)
{
   float3 light_direction;
   float attenuation;

   if (light_position.w == 0) {
      light_direction = -light_position.xyz;
      attenuation = 1.0;
   }
   else {
      light_direction = light_position.xyz - world_position;

      float distance = length(light_direction);

      light_direction = normalize(light_direction);

      attenuation = 1.0 - (distance * distance) / (radius * radius);
      attenuation = saturate(attenuation);
      attenuation *= attenuation;
   }

   float intensity = max(dot(world_normal, light_direction), 0.0);
   
   return attenuation * (intensity * light_color);
}

struct Ps_normalmapped_input
{
   float1 fog_eye_distance : DEPTH;

   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;

   float3x3 TBN : TEXCOORD3;
};

float4 normalmapped_ps(Ps_normalmapped_input input,
                       uniform sampler2D normal_map : register(ps, s[0])) : COLOR
{
   float3 normal = tex2D(normal_map, input.texcoords).rgb * 255.0 / 127.0 - 128.0 / 127.0;

   normal = normalize(mul(input.TBN, normal));

   float3 color = input.static_lighting;
   color += light::ambient(normal);
   color *= light_ambient_color_top.a;

   [unroll] for (uint i = 0; i < light_count; ++i) {
      color += calculate_light(input.world_position, normal,
                               light_positions[i], light_colors[i], 
                               get_light_radius(light_params[i]));
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

struct Ps_perpixel_input
{
   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float3 world_normal : TEXCOORD3;
};

float4 perpixel_ps(Ps_perpixel_input input) : COLOR
{
   float3 normal = normalize(input.world_normal);

   float3 color = input.static_lighting;
   color += light::ambient(normal);
   color *= light_ambient_color_top.a;

   [unroll] for (uint i = 0; i < light_count; ++i) {

      color += calculate_light(input.world_position, normal,
                               light_positions[i], light_colors[i], 
                               get_light_radius(light_params[i]));
   }

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float3 calculate_spotlight(float3 world_position, float3 world_normal, 
                           float3 light_position, float3 light_direction, 
                           float3 light_color, float range,
                           float3 projection_color)
{
   float3 light_normal = normalize(light_position - world_position);
   float intensity = max(dot(world_normal, light_normal), 0.0);

   float light_distance = distance(world_position, light_position);

   float attenuation = dot(light_direction, -light_normal);
   attenuation -= light_distance * light_distance / (range * range);
   attenuation = saturate(attenuation);

   return attenuation * (intensity * light_color) * projection_color;
}

struct Ps_normalmapped_spotlight_input
{
   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float4 projection_coords : TEXCOORD3;

   float3x3 TBN : TEXCOORD4;
};

float4 normalmapped_spotlight_ps(Ps_normalmapped_spotlight_input input,
                                 uniform sampler2D normal_map : register(ps, s[0]),
                                 uniform sampler2D projection_map : register(ps, s[2])) : COLOR
{
   float3 projection_color = sample_projected_light(projection_map,
                                                    input.projection_coords);

   float3 normal = tex2D(normal_map, input.texcoords).rgb * 255.0 / 127.0 - 128.0 / 127.0;

   normal = normalize(mul(input.TBN, normal));

   float3 color = input.static_lighting;
   color += light::ambient(normal);
   color *= light_ambient_color_top.a;

   color += calculate_spotlight(input.world_position, normal, spotlight_position,
                                spotlight_direction, light_colors[0],
                                get_light_radius(spotlight_params), projection_color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

struct Ps_perpixel_spotlight_input
{
   float1 fog_eye_distance : DEPTH;
   float3 world_position : TEXCOORD0;
   float2 texcoords : TEXCOORD1;
   float3 static_lighting : TEXCOORD2;
   float3 world_normal : TEXCOORD3;
   float4 projection_coords : TEXCOORD4;
};

float4 perpixel_spotlight_ps(Ps_perpixel_spotlight_input input,
                             uniform sampler2D projection_map : register(ps, s[2])) : COLOR
{
   float3 projection_color = sample_projected_light(projection_map,
                                                    input.projection_coords);

   float3 normal = normalize(input.world_normal);

   float3 color = input.static_lighting;
   color += light::ambient(normal);
   color *= light_ambient_color_top.a;

   color += calculate_spotlight(input.world_position, normal, spotlight_position,
                                spotlight_direction, light_colors[0],
                                get_light_radius(spotlight_params), projection_color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}
