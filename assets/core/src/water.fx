
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"

float4 texcoords_projection : register(vs, c[CUSTOM_CONST_MIN]);
float4 fade_constant : register(c[CUSTOM_CONST_MIN + 1]);
float4 light_direction : register(c[CUSTOM_CONST_MIN + 2]);
float4 texcoords_tranforms[8] : register(vs, c[CUSTOM_CONST_MIN + 3]);

const static float4 x_texcoords_transform[4] = {texcoords_tranforms[0], texcoords_tranforms[2],
                                                texcoords_tranforms[4], texcoords_tranforms[6]};
const static float4 y_texcoords_transform[4] = {texcoords_tranforms[1], texcoords_tranforms[3],
                                                texcoords_tranforms[5], texcoords_tranforms[7]};

float4 refraction_colour : register(ps, c[0]);
float4 reflection_colour : register(ps, c[1]);
float4 fresnel_min_max : register(ps, c[2]);
float4 blend_map_constant : register(ps, c[3]);
float4 blend_specular_constant : register(ps, c[4]);

const static float water_fade = saturate(fade_constant.z + fade_constant.w);
const static float4 time_scales = {0.009, 0.01, 0.008, 0.01};
const static float4 tex_scales = {0.03, 0.04, 0.01, 0.04};
const static float2 water_direction = {0.5, 1.0};
const static float specular_exponent = 256;
const static float bump_scale = 0.075;

Texture2D<float2> normal_map_texture : register(ps_3_0, s12);
Texture2D<float3> refraction_buffer : register(ps_3_0, s13);

SamplerState linear_wrap_sampler;

float4 discard_vs() : SV_Position
{
   return 0.0;
}

struct Vs_fade_output
{
   float4 positionPS : SV_Position;
   float fade : TEXCOORD0;
   float fog_eye_distance : DEPTH;
};

Vs_fade_output transmissive_pass_fade_vs(Vertex_input input)
{
   Vs_fade_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fade = saturate(positionPS.z * fade_constant.z + fade_constant.w);
   output.fog_eye_distance = fog::get_eye_distance(positionWS);

   return output;
}

struct Vs_lowquality_output
{
   float4 positionPS : POSITION;
   float2 diffuse_texcoords[2] : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD2;
   float2 specular_fade : TEXCOORD4;
   float1 fog_eye_distance: TEXCOORD5;
};

Vs_lowquality_output lowquality_vs(Vertex_input input)
{
   Vs_lowquality_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float3 normalWS = normalize(transformer.normalWS());
   const float3 view_normalWS = normalize(view_positionWS - positionWS);

   const float3 half_vectorWS = normalize(light_direction.xyz + view_normalWS);
   const float HdotN = max(dot(half_vectorWS, normalWS), 0.0);
   output.specular_fade.x = pow(HdotN, light_direction.w);

   output.diffuse_texcoords[0] = transformer.texcoords(x_texcoords_transform[0], 
                                                       y_texcoords_transform[0]);
   output.diffuse_texcoords[1] = transformer.texcoords(x_texcoords_transform[1], 
                                                       y_texcoords_transform[1]);

   output.spec_texcoords[0] = transformer.texcoords(x_texcoords_transform[2], 
                                                    y_texcoords_transform[2]);
   output.spec_texcoords[1] = transformer.texcoords(x_texcoords_transform[3], 
                                                    y_texcoords_transform[3]);

   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fog_eye_distance = fog::get_eye_distance(positionWS);
   output.specular_fade.y = saturate(positionPS.z * fade_constant.z + fade_constant.w);

   return output;
}

struct Vs_normal_map_output
{
   float4 positionPS : SV_Position;
   float3 view_normalWS : TEXCOORD0;
   float2 texcoords[4] : TEXCOORD1;
   float2 fade_fog_eye_distance : TEXCOORD5;
};

Vs_normal_map_output normal_map_vs(Vertex_input input)
{
   Vs_normal_map_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.view_normalWS = view_positionWS - positionWS;
   output.fade_fog_eye_distance.y = fog::get_eye_distance(positionWS);

   float2 texcoords = positionWS.xz;

   const float2x2 rotation = {{cos(0.785398), -sin(0.785398)}, {sin(0.785398), cos(0.785398)}};

   output.texcoords[0] = (texcoords * tex_scales[0]) + (time_scales[0] * time);
   output.texcoords[1] = (texcoords * tex_scales[1]) * float2(0.5, 1.0) + (time_scales[1] * time);
   output.texcoords[2] = (texcoords * tex_scales[2]) * float2(0.125, 1.0) + (time_scales[2] * time);
   output.texcoords[3] = mul(rotation, texcoords * tex_scales[3]) + (time_scales[3] * time);

   output.fade_fog_eye_distance.x = saturate(positionPS.z * fade_constant.z + fade_constant.w);

   return output;
}

// Pixel Shaders //


float calc_fresnel(float normal_view_dot)
{
   float fresnel = (1.0 - normal_view_dot);
   fresnel *= fresnel;

   return lerp(fresnel_min_max.z, fresnel_min_max.w, fresnel);
}

float4 transmissive_pass_fade_ps(float fade : TEXCOORD0, float fog_eye_distance : DEPTH) : SV_Target0
{
   return float4(fog::apply(refraction_colour.rgb * fade, fog_eye_distance), fade);
}

struct Ps_normal_map_input
{
   float2 position : VPOS;
   float3 view_normalWS : TEXCOORD0;
   float2 texcoords[4] : TEXCOORD1;
   float2 fade_fog_eye_distance : TEXCOORD5;
};

float3 sample_normal_map(float2 texcoords)
{
   float3 normal;
   
   normal.xy = normal_map_texture.SampleBias(linear_wrap_sampler, texcoords, -1.0).xy;
   normal.xy = normal.xy * 255.0 / 127.0 - 128.0 / 127.0;
   normal.z = sqrt(1.0 - saturate(normal.x * normal.x - normal.y * normal.y));

   return normal.xzy;
}

void sample_normal_maps(float2 texcoords[4], out float2 bump_out, out float3 normalWS_out)
{
   float3 normal = sample_normal_map(texcoords[0]);
   normal += sample_normal_map(texcoords[1]);
   normal += sample_normal_map(texcoords[2]);
   normal += sample_normal_map(texcoords[3]);

   bump_out = normal.xz * 0.25 * bump_scale;
   normalWS_out = normalize(normal);
}

float4 normal_map_distorted_reflection_ps(Ps_normal_map_input input,
                                          Texture2D<float3> reflection_buffer : register(ps_3_0, s3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += bump;

   const float3 reflection = 
      reflection_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);
   const float3 refraction = 
      refraction_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));

   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   color = fog::apply(color, input.fade_fog_eye_distance.y);

   return float4(color, input.fade_fog_eye_distance.x);
}

float4 normal_map_distorted_reflection_specular_ps(Ps_normal_map_input input,
                                                   Texture2D<float3> reflection_buffer : register(ps_3_0, s3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += bump;

   const float3 reflection =
      reflection_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);
   const float3 refraction =
      refraction_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));
   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   const float3 half_normalWS = normalize(light_direction.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, half_normalWS));
   color += pow(NdotH, specular_exponent);

   color = fog::apply(color, input.fade_fog_eye_distance.y);

   return float4(color, input.fade_fog_eye_distance.x);
}

float4 normal_map_distorted_ps(Ps_normal_map_input input,
                               Texture2D<float3> reflection_buffer : register(ps_3_0, s3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += bump;

   const float3 reflection =
      reflection_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);
   const float3 refraction =
      refraction_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));

   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   color = fog::apply(color, input.fade_fog_eye_distance.y);

   return float4(color, input.fade_fog_eye_distance.x);
}

float4 normal_map_distorted_specular_ps(Ps_normal_map_input input,
                                        Texture2D<float3> reflection_buffer : register(ps_3_0, s3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   float2 reflection_coords = input.position * rt_resolution.zw;
   reflection_coords += bump;

   const float3 reflection =
      reflection_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);
   const float3 refraction =
      refraction_buffer.SampleLevel(linear_wrap_sampler, reflection_coords, 0);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));
   
   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   const float3 half_normalWS = normalize(light_direction.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, half_normalWS));
   color += pow(NdotH, specular_exponent);

   color = fog::apply(color, input.fade_fog_eye_distance.y);

   return float4(color, input.fade_fog_eye_distance.x);
}

float4 normal_map_ps(Ps_normal_map_input input) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 color = fog::apply(reflection_colour.rgb, input.fade_fog_eye_distance.y);

   return float4(color, fresnel * input.fade_fog_eye_distance.x);
}

float4 normal_map_specular_ps(Ps_normal_map_input input) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   const float3 view_normalWS = normalize(input.view_normalWS);
   const float VdotN = max(dot(view_normalWS, normalWS), 0.0);

   const float fresnel = calc_fresnel(VdotN);

   const float3 half_normalWS = normalize(light_direction.xyz + view_normalWS);
   const float NdotH = saturate(dot(normalWS, half_normalWS));
   const float spec = pow(NdotH, specular_exponent);

   const float3 color = fog::apply(reflection_colour.rgb + spec, input.fade_fog_eye_distance.y);

   return float4(color, fresnel * input.fade_fog_eye_distance.x);
}

struct Ps_lowquality_input
{
   float2 diffuse_texcoords[2] : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD2;
   float2 specular_fade : TEXCOORD4;
   float1 fog_eye_distance: TEXCOORD5;
};

float4 lowquality_ps(Ps_lowquality_input input,
                     Texture2D<float4> diffuse_map_texture : register(ps_3_0, s1)) : SV_Target0
{
   const float4 diffuse_color = 
      diffuse_map_texture.Sample(linear_wrap_sampler, input.diffuse_texcoords[1]);

   float4 color = refraction_colour * diffuse_color;
   color.rgb *= hdr_info.z;
   color.a *= input.specular_fade.y;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 lowquality_specular_ps(Ps_lowquality_input input,
                              Texture2D<float4> diffuse_map_texture : register(ps_3_0, s1),
                              Texture2D<float3> specular_mask_textures[2] : register(ps_3_0, s2)) : SV_Target0
{
   const float4 diffuse_color =
      diffuse_map_texture.Sample(linear_wrap_sampler, input.diffuse_texcoords[1]);

   const float3 spec_mask_0 = 
      specular_mask_textures[0].Sample(linear_wrap_sampler, input.spec_texcoords[0]);
   const float3 spec_mask_1 =
      specular_mask_textures[1].Sample(linear_wrap_sampler, input.spec_texcoords[1]);
   const float3 spec_mask = lerp(spec_mask_0, spec_mask_1, blend_specular_constant.rgb);

   float4 color = refraction_colour * diffuse_color;

   color.rgb += (spec_mask * input.specular_fade.x);
   color.rgb *= hdr_info.z;
   color.a *= input.specular_fade.y;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 discard_ps() : SV_Target0
{
   discard;

   return 0.0;
}
