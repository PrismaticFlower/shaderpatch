
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"

float4 texcoords_projection : register(vs, c[CUSTOM_CONST_MIN]);
float4 fade_constant : register(c[CUSTOM_CONST_MIN + 1]);
float4 light_direction : register(c[CUSTOM_CONST_MIN + 2]);
float4 texture_transforms[8] : register(vs, c[CUSTOM_CONST_MIN + 3]);

const static float water_fade = saturate(fade_constant.z + fade_constant.w);
const static float wave_length = 0.1;
const static float wave_height = 0.1;
const static float2 water_direction = {0.5, 1.0};
const static float time_scale = 0.003333;
const static float specular_exponent = 128;

float4 discard_vs() : POSITION
{
   return 0.0;
}

struct Vs_fade_output
{
   float4 position : POSITION;
   float fog_eye_distance : DEPTH;
   float fade : TEXCOORD0;
};

Vs_fade_output transmissive_pass_fade_vs(float4 position : POSITION)
{
   Vs_fade_output output;

   float4 world_position = transform::position(position);

   output.position = position_project(world_position);
   output.fade = output.position.z * fade_constant.z + fade_constant.w;
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   return output;
}

struct Vs_lowquality_output
{
   float4 position : POSITION;
   float fog : FOG;
   float2 diffuse_texcoords[2] : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD2;
   float2 hdr_scale_fade : COLOR0;
   float specular : COLOR1;
};

Vs_lowquality_output lowquality_vs(float4 position : POSITION, float3 normal : NORMAL,
                                   float4 texcoords : TEXCOORD)
{
   Vs_lowquality_output output;

   float4 world_position = transform::position(position);
   float3 world_normal = normals_to_world(decompress_normals(normal));

   float3 view_normal = normalize(world_view_position - world_position).xyz;

   float3 half_vector = normalize(light_direction.xyz + view_normal);
   float specular_angle = max(dot(half_vector, normal), 0.0);
   output.specular = pow(specular_angle, light_direction.w);

   output.diffuse_texcoords[0] = decompress_transform_texcoords(texcoords,
                                                                texture_transforms[0],
                                                                texture_transforms[1]);
   output.diffuse_texcoords[1] = decompress_transform_texcoords(texcoords,
                                                                texture_transforms[2],
                                                                texture_transforms[3]);

   output.spec_texcoords[0] = decompress_transform_texcoords(texcoords,
                                                             texture_transforms[4],
                                                             texture_transforms[5]);
   output.spec_texcoords[1] = decompress_transform_texcoords(texcoords,
                                                             texture_transforms[6],
                                                             texture_transforms[7]);

   output.position = position_project(world_position);
   output.hdr_scale_fade.x = hdr_info.z;
   output.hdr_scale_fade.y = output.position.z * fade_constant.z + fade_constant.w;
   output.fog = calculate_fog(world_position);

   return output;
}

struct Vs_normal_map_output
{
   float4 position : POSITION;
   float fog_eye_distance : DEPTH;
   float fade : COLOR0;
   float3 view_normal : TEXCOORD0;
   float3 world_normal : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
};

Vs_normal_map_output normal_map_vs(float4 position : POSITION, float3 normal : NORMAL)
{
   Vs_normal_map_output output;

   float4 world_position = transform::position(position);

   output.position = position_project(world_position);
   output.world_normal = normals_to_world(decompress_normals(normal));
   output.view_normal = normalize(world_view_position - world_position).xyz;
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   float2 texcoords = world_position.xz * 0.125;

   output.texcoords = texcoords * wave_length + time_scale * time * water_direction;
   output.fade = output.position.z * fade_constant.z + fade_constant.w;

   return output;
}

// Pixel Shaders //

float4 refraction_colour : register(ps, c[0]);
float4 reflection_colour : register(ps, c[1]);
float4 fresnel_min_max : register(ps, c[2]);
float4 blend_map_constant : register(ps, c[3]);
float4 blend_specular_constant : register(ps, c[4]);

float calc_fresnel(float normal_view_dot)
{
   float fresnel = (1.0 - normal_view_dot);
   fresnel *= fresnel;

   return lerp(fresnel_min_max.z, fresnel_min_max.w, fresnel);
}

float4 transmissive_pass_fade_ps(float fade : TEXCOORD0, float fog_eye_distance : DEPTH) : COLOR
{
   return float4(fog::apply(refraction_colour.rgb * fade, fog_eye_distance), fade);
}

struct Ps_normal_map_input
{
   float2 position : VPOS;
   float fog_eye_distance : DEPTH;
   float fade : COLOR0;
   float3 view_normal : TEXCOORD0;
   float3 world_normal : TEXCOORD1;
   float2 texcoords : TEXCOORD2;
};

float4 normal_map_distorted_reflection_ps(Ps_normal_map_input input,
                                          uniform sampler2D normal_map : register(s8),
                                          uniform sampler2D refraction_map : register(s9),
                                          uniform sampler2D reflection_map : register(s3)) : COLOR
{
   float3 view_normal = normalize(input.view_normal);

   float2 bump = tex2D(normal_map, input.texcoords).xy  * 2.0 - 1.0;
   float3 normal = perturb_normal(normal_map, input.texcoords, 
                                  normalize(input.world_normal), view_normal);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += (bump * wave_height);

   float3 reflection = tex2D(reflection_map, reflection_coords).rgb;
   float3 refraction = tex2D(refraction_map, reflection_coords).rgb;

   float normal_view_dot = max(dot(view_normal, normal), 0.0);

   float fresnel = calc_fresnel(normal_view_dot);

   float3 water_color = 
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));

   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float4 normal_map_distorted_reflection_specular_ps(Ps_normal_map_input input,
                                                   uniform sampler2D normal_map : register(s8),
                                                   uniform sampler2D refraction_map : register(s9),
                                                   uniform sampler2D reflection_map : register(s3)) : COLOR
{
   float3 view_normal = normalize(input.view_normal);

   float2 bump = tex2D(normal_map, input.texcoords).xy  * 2.0 - 1.0;
   float3 normal = perturb_normal(normal_map, input.texcoords,
                                  normalize(input.world_normal), view_normal);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += (bump * wave_height);

   float3 reflection = tex2D(reflection_map, reflection_coords).rgb;
   float3 refraction = tex2D(refraction_map, reflection_coords).rgb;

   float fresnel = calc_fresnel(max(dot(view_normal, normal), 0.0));

   float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));

   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   float3 half_vector = normalize(light_direction.xyz + view_normal);

   float specular_angle = max(dot(half_vector, normal), 0.0);
   float specular = pow(specular_angle, specular_exponent);

   color += specular;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, 1.0);
}

float4 normal_map_reflection_ps(Ps_normal_map_input input,
                                           uniform sampler2D normal_map : register(s8),
                                           uniform sampler2D reflection_map : register(s3)) : COLOR
{
   float3 reflection = tex2D(reflection_map, input.position * rt_resolution.zw).rgb;
   
   float3 view_normal = normalize(input.view_normal);
   float3 normal = perturb_normal(normal_map, input.texcoords,
                                  normalize(input.world_normal), view_normal);

   float normal_view_dot = dot(normal, view_normal);

   float fresnel = calc_fresnel(max(dot(view_normal, normal), 0.0));

   float3 color = reflection * reflection_colour.a;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, fresnel * input.fade);
}

float4 normal_map_reflection_specular_ps(Ps_normal_map_input input,
                                         uniform sampler2D normal_map : register(s8), 
                                         uniform sampler2D reflection_map : register(s3)) : COLOR
{
   float3 reflection = tex2D(reflection_map, input.position * rt_resolution.zw).rgb;
   
   float3 view_normal = normalize(input.view_normal);
   float3 normal = perturb_normal(normal_map, input.texcoords,
                                  normalize(input.world_normal), view_normal);

   float fresnel = calc_fresnel(max(dot(view_normal, normal), 0.0));

   float3 half_vector = normalize(light_direction.xyz + view_normal);

   float specular_angle = saturate(dot(half_vector, normal));
   float specular = pow(specular_angle, specular_exponent);

   float3 color = reflection * reflection_colour.a + specular;

   color = fog::apply(color, input.fog_eye_distance);

   return float4(color, fresnel * input.fade);
}

float4 normal_map_ps(Ps_normal_map_input input,
                     uniform sampler2D normal_map : register(s8)) : COLOR
{
   float3 view_normal = normalize(input.view_normal);
   float3 normal = perturb_normal(normal_map, input.texcoords,
                                  normalize(input.world_normal), view_normal);

   float fresnel = calc_fresnel(max(dot(view_normal, normal), 0.0));

   float3 color = fog::apply(reflection_colour.rgb, input.fog_eye_distance);

   return float4(color, fresnel * input.fade);
}

float4 normal_map_specular_ps(Ps_normal_map_input input,
                              uniform sampler2D normal_map : register(s8)) : COLOR
{
   float3 view_normal = normalize(input.view_normal);
   float3 normal = perturb_normal(normal_map, input.texcoords,
                                  normalize(input.world_normal), view_normal);

   float fresnel = calc_fresnel(max(dot(view_normal, normal), 0.0));

   float3 half_vector = normalize(light_direction.xyz + view_normal);

   float specular_angle = saturate(dot(half_vector, normal));
   float specular = pow(specular_angle, specular_exponent);

   float3 color = fog::apply(reflection_colour.rgb + specular, input.fog_eye_distance);

   return float4(color, fresnel * input.fade);
}

struct Ps_lowquality_input
{
   float2 diffuse_texcoords[2] : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD2;
   float2 hdr_scale_fade : COLOR0;
   float specular : COLOR1;
};

float4 lowquality_ps(Ps_lowquality_input input,
                     uniform sampler2D diffuse_map : register(s1)) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords[1]);

   float4 color = refraction_colour * diffuse_color;
   color.rgb *= input.hdr_scale_fade.x;
   color.a *= input.hdr_scale_fade.y;

   return color;
}

float4 lowquality_specular_ps(Ps_lowquality_input input,
                              uniform sampler2D diffuse_map : register(s1),
                              uniform sampler2D specular_map[2] : register(s2)) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords[1]);

   float3 spec_mask_0 = tex2D(specular_map[0], input.spec_texcoords[0]).rgb;
   float3 spec_mask_1 = tex2D(specular_map[1], input.spec_texcoords[1]).rgb;
   float3 spec_mask = lerp(spec_mask_0, spec_mask_1, blend_specular_constant.rgb);

   float4 color = refraction_colour * diffuse_color;

   color.rgb += (spec_mask * input.specular);
   color.rgb *= input.hdr_scale_fade.x;
   color.a *= input.hdr_scale_fade.y;

   return color;
}
