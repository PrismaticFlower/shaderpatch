
#include "constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"

static const float specular_exponent = 16.0;

// I personally feel a high exponent gives nicer results
// for normal mapped models. You might very reasonably
// disagree, in which case feel free to change this!
static const float normal_map_specular_exponent = 128.0;

float4 texture_transforms[2] : register(vs, c[CUSTOM_CONST_MIN]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   float3 binormal : BINORMAL;
   float3 tangent : TANGENT;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
};

struct Vs_normalmapped_ouput
{
   float4 position : POSITION;

   float2 texcoords : TEXCOORD0;

   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float3 world_view_position : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

Vs_normalmapped_ouput normalmapped_vs(Vs_input input)
{
   Vs_normalmapped_ouput output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.world_view_position = world_view_position.xyz;
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

   return output;
}

struct Vs_blinn_phong_ouput
{
   float4 position : POSITION;

   float2 texcoords : TEXCOORD0;

   float3 world_position : TEXCOORD1;
   float3 world_view_position : TEXCOORD2;
   float3 normal : TEXCOORD3;

   float3 envmap_coords : TEXCOORD4;
   float envmapped : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

Vs_blinn_phong_ouput blinn_phong_vs(Vs_input input,
                                    uniform float4 specular_state : register(vs, c[CUSTOM_CONST_MIN + 2]))
{
   Vs_blinn_phong_ouput output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.world_view_position = world_view_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   float3 world_normal = normals_to_world(transform::normals(input.normals,
                                                             input.blend_indices,
                                                             input.weights));

   output.normal = world_normal;

   float3 camera_direction = normalize(world_view_position.xyz - world_position.xyz);
   output.envmap_coords = normalize(reflect(world_normal, camera_direction));
   output.envmapped = (specular_state.x == 1.0);

   return output;
}

struct Vs_normalmapped_envmap_ouput
{
   float4 position : POSITION;

   float2 texcoords : TEXCOORD0;
   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_view_position : TEXCOORD5;
   float1 fog_eye_distance : DEPTH;
};

Vs_normalmapped_envmap_ouput normalmapped_envmap_vs(Vs_input input)
{
   Vs_normalmapped_envmap_ouput output;

   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.world_position = world_position.xyz;
   output.world_view_position = world_view_position.xyz;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.texcoords = decompress_transform_texcoords(input.texcoords,
                                                     texture_transforms[0],
                                                     texture_transforms[1]);

   float3 world_normal = normals_to_world(transform::normals(input.normals,
                                          input.blend_indices,
                                          input.weights));

   Binormals world_binormals = binormals_to_world(transform::binormals(input.binormal,
                                                                       input.tangent,
                                                                       input.blend_indices,
                                                                       input.weights));

   output.normal = world_normal;
   output.binormal = world_binormals.s;
   output.tangent = world_binormals.t;

   return output;
}

float3 calculate_blinn_phong(float3 normal, float3 view_normal, float3 world_position,
                             float4 light_position, float3 light_color, 
                             float3 specular_color, float exponent)
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

   float3 half_vector = normalize(light_direction + view_normal);
   float specular_angle = max(dot(half_vector, normal), 0.0);
   float specular = pow(specular_angle, exponent);

   return attenuation * (specular_color * light_color * specular);
}

struct Ps_normalmapped_input
{
   float2 texcoords : TEXCOORD0;
   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_view_position : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

float4 normalmapped_ps(Ps_normalmapped_input input,
                       uniform sampler2D normal_map,
                       uniform float4 specular_color : register(ps, c[0]),
                       uniform float3 light_color : register(ps, c[2]),
                       uniform float4 light_position : register(c[CUSTOM_CONST_MIN + 2])) : COLOR
{
   float4 normal_map_color = tex2D(normal_map, input.texcoords);

   float3 texel_normal = normal_map_color.rgb * 2.0 - 1.0;

   texel_normal = normalize(texel_normal.x * input.tangent -
                            texel_normal.y * input.binormal +
                            texel_normal.z * input.normal);

   float3 view_normal = normalize(input.world_view_position - input.world_position);

   float3 spec_color = calculate_blinn_phong(texel_normal, view_normal, input.world_position,
                                             light_position, light_color,
                                             specular_color.rgb, 
                                             normal_map_specular_exponent);

   float gloss = lerp(1.0, normal_map_color.a, specular_color.a);
   float3 color = gloss * spec_color;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_color.a);
}

struct Ps_blinn_phong_input
{
   float2 texcoords : TEXCOORD0;

   float3 world_position : TEXCOORD1;
   float3 world_view_position : TEXCOORD2;
   float3 normal : TEXCOORD3;

   float3 envmap_coords : TEXCOORD4;
   float envmapped : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

float4 blinn_phong_ps(Ps_blinn_phong_input input, sampler2D diffuse_map,
                      samplerCUBE envmap, float4 specular_color, float3 light_colors[3],
                      float4 light_positions[3], const int light_count)
{
   float diffuse_alpha = tex2D(diffuse_map, input.texcoords).a;
   float gloss = lerp(1.0, diffuse_alpha, specular_color.a);

   float3 normal = normalize(input.normal);
   float3 view_normal = normalize(input.world_view_position - input.world_position);

   float3 spec_color = float3(0.0, 0.0, 0.0);

   if (light_count >= 1 && !input.envmapped) {
      spec_color += calculate_blinn_phong(normal, view_normal, input.world_position,
                                          light_positions[0], light_colors[0], 
                                          specular_color.rgb, specular_exponent);
   }
   
   if (light_count >= 2) {
      spec_color += calculate_blinn_phong(normal, view_normal, input.world_position,
                                          light_positions[1], light_colors[1], 
                                          specular_color.rgb, specular_exponent);
   }
   
   if (light_count >= 3) {
      spec_color += calculate_blinn_phong(normal, view_normal, input.world_position,
                                          light_positions[2], light_colors[2], 
                                          specular_color.rgb, specular_exponent);
   }

   float3 envmap_color = texCUBE(envmap, input.envmap_coords).rgb * specular_color.rgb;
   float3 color = saturate(gloss * (envmap_color + spec_color));

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, diffuse_alpha);
}

float4 blinn_phong_lights_3_ps(Ps_blinn_phong_input input,
                               uniform sampler2D diffuse_map, uniform samplerCUBE envmap,
                               uniform float4 specular_color : register(ps, c[0]),
                               uniform float3 light_colors[3] : register(ps, c[2]),
                               uniform float4 light_positions[3] : register(c[CUSTOM_CONST_MIN + 3])) : COLOR
{
   return blinn_phong_ps(input, diffuse_map, envmap, specular_color, light_colors,  
                         light_positions, 3);
}

float4 blinn_phong_lights_2_ps(Ps_blinn_phong_input input,
                               uniform sampler2D diffuse_map, uniform samplerCUBE envmap,
                               uniform float4 specular_color : register(ps, c[0]),
                               uniform float3 light_colors[3] : register(ps, c[2]),
                               uniform float4 light_positions[3] : register(c[CUSTOM_CONST_MIN + 3])) : COLOR
{
   return blinn_phong_ps(input, diffuse_map, envmap, specular_color, light_colors,  
                         light_positions, 2);
}

float4 blinn_phong_lights_1_ps(Ps_blinn_phong_input input,
                               uniform sampler2D diffuse_map, uniform samplerCUBE envmap,
                               uniform float4 specular_color : register(ps, c[0]),
                               uniform float3 light_colors[3] : register(ps, c[2]),
                               uniform float4 light_positions[3] : register(c[CUSTOM_CONST_MIN + 3])) : COLOR
{
   return blinn_phong_ps(input, diffuse_map, envmap, specular_color, light_colors,  
                         light_positions, 1);
}

struct Ps_normalmapped_envmap_input
{
   float2 texcoords : TEXCOORD0;
   float3 normal : TEXCOORD1;
   float3 binormal : TEXCOORD2;
   float3 tangent : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_view_position : TEXCOORD5;

   float1 fog_eye_distance : DEPTH;
};

float4 normalmapped_envmap_ps(Ps_normalmapped_envmap_input input,
                              uniform sampler2D normal_map, 
                              uniform samplerCUBE envmap : register(ps, s[3]),
                              uniform float4 specular_color : register(ps, c[0]),
                              uniform float3 light_color : register(ps, c[2])) : COLOR
{
   float4 normal_map_color = tex2D(normal_map, input.texcoords);

   float3 texel_normal = normal_map_color.rgb * 2.0 - 1.0;

   texel_normal = normalize(texel_normal.x * input.tangent -
                            texel_normal.y * input.binormal +
                            texel_normal.z * input.normal);

   float3 camera_direction = normalize(input.world_view_position - input.world_position);
   float3 envmap_coords = normalize(reflect(texel_normal, camera_direction));
   float3 envmap_color = texCUBE(envmap, envmap_coords).rgb;

   float gloss = lerp(1.0, normal_map_color.a, specular_color.a);
   float3 color = light_color * envmap_color * gloss;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, normal_map_color.a);
}

float4 debug_vertexlit_ps() : COLOR
{
   return float4(1.0, 1.0, 0.0, 1.0);
}
