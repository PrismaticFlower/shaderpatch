
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

// Disbale single loop iteration warning.
#pragma warning(disable : 3557)

float4 texture_transforms[2] : register(vs, c[CUSTOM_CONST_MIN]);

float4 light_constants[7] : register(c[21]);

const static float4 light_positions[3] = { light_constants[0], light_constants[2], light_constants[4] };
const static float4 light_params[3] = { light_constants[1], light_constants[3], light_constants[5] };

const static float3 spotlight_position =  light_constants[0].xyz;
const static float4 spotlight_params = light_constants[1];
const static float3 spotlight_direction = light_constants[2].xyz;

Binormals generate_birnormals(float3 world_normal)
{
   Binormals binormals;

   //Pandemic's note: we rely on the fact that the object is world axis aligned
   binormals.s = -world_normal.x * world_normal + float3(1.0, 0.0, 0.0);
   binormals.s *= rsqrt(binormals.s.x);

   binormals.t = -world_normal.z * world_normal + float3(0.0, 0.0, 1.0);
   binormals.t *= rsqrt(binormals.t.z);

   return binormals;
}

float get_light_radius(float4 light_params)
{
   return (1.0 / light_params.x) * light_params.y;
}

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   float3 binormal : BINORMAL;
   float3 tangent : TANGENT;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_3lights_output
{
   float4 position : POSITION;
   float1 fog_eye_distance : DEPTH;

   float3 ambient_color : COLOR;

   float2 texcoords : TEXCOORD0;

   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;

   float3 world_position : TEXCOORD4;
};

Vs_3lights_output lights_3_vs(Vs_input input)
{
   Vs_3lights_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   float3 world_normals = normals_to_world(transform::normals(input.normals,
                                           input.blend_indices,
                                           input.weights));

   Binormals world_binormals = binormals_to_world(transform::binormals(input.binormal,
                                                  input.tangent,
                                                  input.blend_indices,
                                                  input.weights));

   output.normal = world_normals;
   output.binormal = world_binormals.s;
   output.tangent = world_binormals.t;

   float4 static_diffuse_color = get_static_diffuse_color(input.color);

   float3 ambient_light = light::ambient(world_normals);
   ambient_light += static_diffuse_color.rgb;
   output.ambient_color = ambient_light * light_ambient_color_top.a;

   return output;
}

Vs_3lights_output lights_3_genbinormals_vs(Vs_input input)
{
   Vs_3lights_output output = lights_3_vs(input);

   Binormals world_binormals = generate_birnormals(output.normal);

   output.binormal = world_binormals.s;
   output.tangent = world_binormals.t;

   return output;
}

Vs_3lights_output lights_3_genbinormals_terrain_vs(Vs_input input)
{
   Vs_3lights_output output = lights_3_genbinormals_vs(input);

   output.texcoords.x = dot(float4(output.world_position, 1.0), texture_transforms[0].xzyw);
   output.texcoords.y = dot(float4(output.world_position, 1.0), texture_transforms[1].xzyw);

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

struct Vs_spotlight_output
{
   float4 position : POSITION;
   float1 fog_eye_distance : DEPTH;
   float3 ambient_color : COLOR;
   float2 texcoords : TEXCOORD0;

   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float4 projection_coords : TEXCOORD5;
};

Vs_spotlight_output spotlight_vs(Vs_input input)
{
   Vs_spotlight_output output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   float3 world_normals = normals_to_world(transform::normals(input.normals,
                                           input.blend_indices,
                                           input.weights));

   Binormals world_binormals = binormals_to_world(transform::binormals(input.binormal,
                                                  input.tangent,
                                                  input.blend_indices,
                                                  input.weights));

   output.normal = world_normals;
   output.binormal = world_binormals.s;
   output.tangent = world_binormals.t;

   float4 static_diffuse_color = get_static_diffuse_color(input.color);

   float3 ambient_light = light::ambient(world_normals);
   ambient_light += static_diffuse_color.rgb;
   output.ambient_color = ambient_light * light_ambient_color_top.a;

   output.projection_coords = transform_spotlight_projection(world_position);

   return output;
}

Vs_spotlight_output spotlight_genbinormals_vs(Vs_input input)
{
   Vs_spotlight_output output = spotlight_vs(input);

   Binormals world_binormals = generate_birnormals(output.normal);

   output.binormal = world_binormals.s;
   output.tangent = world_binormals.t;

   return output;
}

Vs_spotlight_output spotlight_genbinormals_terrain_vs(Vs_input input)
{
   Vs_spotlight_output output = spotlight_genbinormals_vs(input);

   output.texcoords.x = dot(float4(output.world_position, 1.0), texture_transforms[0].xzyw);
   output.texcoords.y = dot(float4(output.world_position, 1.0), texture_transforms[1].xzyw);

   return output;
}

struct Ps_3lights_input
{
   float3 ambient_color : COLOR;

   float2 texcoords : TEXCOORD0;

   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;

   float3 world_position : TEXCOORD4;

   float1 fog_eye_distance : DEPTH;
};

float3 light_colors[3] : register(ps, c[0]);

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

float4 lights_normalmap_ps(Ps_3lights_input input, sampler2D normal_map,
                           const int light_count)
{
   float3 texel_normal = tex2D(normal_map, input.texcoords).rgb * 2.0 - 1.0;

   texel_normal = normalize(texel_normal.x * input.tangent -
                            texel_normal.y * input.binormal +
                            texel_normal.z * input.normal);

   float3 color = input.ambient_color;

   for (int i = 0; i < light_count; ++i) {
      color += calculate_light(input.world_position, texel_normal, 
                               light_positions[i], light_colors[i], 
                               get_light_radius(light_params[i]));
   }

   color = saturate(color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float4 lights_3_normalmap_ps(Ps_3lights_input input,
                             uniform sampler2D normal_map : register(ps, s[0])) : COLOR
{
   return lights_normalmap_ps(input, normal_map, 3);
}

float4 lights_2_normalmap_ps(Ps_3lights_input input,
                             uniform sampler2D normal_map : register(ps, s[0])) : COLOR
{
   return lights_normalmap_ps(input, normal_map, 2);
}

float4 lights_1_normalmap_ps(Ps_3lights_input input,
                             uniform sampler2D normal_map : register(ps, s[0])) : COLOR
{
   return lights_normalmap_ps(input, normal_map, 1);
}

float4 lights_ps(Ps_3lights_input input, const int light_count)
{
   float3 color = input.ambient_color;

   for (int i = 0; i < light_count; ++i) {

      color += calculate_light(input.world_position, input.normal, 
                               light_positions[i], light_colors[i], 
                               get_light_radius(light_params[i]));
   }

   color = saturate(color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float4 lights_3_ps(Ps_3lights_input input) : COLOR
{
   return lights_ps(input, 3);
}

float4 lights_2_ps(Ps_3lights_input input) : COLOR
{
   return lights_ps(input, 2);
}

float4 lights_1_ps(Ps_3lights_input input) : COLOR
{
   return lights_ps(input, 1);
}

struct Ps_spotlight_input
{
   float3 ambient_color : COLOR;
   float2 texcoords : TEXCOORD0;

   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float4 projection_coords : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

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

float4 spotlight_normalmap_ps(Ps_spotlight_input input,
                              uniform sampler2D normal_map : register(ps, s[0]),
                              uniform sampler2D projection_map : register(ps, s[2])) : COLOR
{
   float3 texel_normal = tex2D(normal_map, input.texcoords).rgb * 2.0 - 1.0;

   texel_normal = normalize(texel_normal.x * input.tangent -
                            texel_normal.y * input.binormal +
                            texel_normal.z * input.normal);

   float3 projection_color = sample_projected_light(projection_map,
                                                    input.projection_coords);

   float3 color = input.ambient_color;

   color += calculate_spotlight(input.world_position, texel_normal, spotlight_position,
                                spotlight_direction, light_colors[0],
                                get_light_radius(spotlight_params), projection_color);
   color = saturate(color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float4 spotlight_ps(Ps_spotlight_input input,
                    uniform sampler2D projection_map : register(ps, s[2])) : COLOR
{
   float3 projection_color = sample_projected_light(projection_map,
                                                    input.projection_coords);

   float3 color = input.ambient_color;

   color += calculate_spotlight(input.world_position, input.normal, spotlight_position,
                                spotlight_direction, light_colors[0],
                                get_light_radius(spotlight_params), projection_color);
   color = saturate(color);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}
