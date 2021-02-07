#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

const static float4 texcoords_projection = custom_constants[0];
const static float4 fade_constant = custom_constants[1];
const static float4 light_direction = custom_constants[2];
const static float4 x_texcoords_transform[3] = {custom_constants[3], custom_constants[5],
                                                custom_constants[7]};
const static float4 y_texcoords_transform[3] = {custom_constants[4], custom_constants[6],
                                                custom_constants[8]};

const static float4 refraction_colour = ps_custom_constants[0];
const static float4 reflection_colour = ps_custom_constants[1];
const static float4 fresnel_min_max = ps_custom_constants[2];
const static float4 blend_map_constant = ps_custom_constants[3];
const static float4 blend_specular_constant = ps_custom_constants[4];

const static float water_fade = saturate(fade_constant.z + fade_constant.w);
const static float4 time_scales = {0.0091666, 0.01, 0.00833, 0.01};
const static float4 tex_scales = {0.03, 0.04, 0.01, 0.04};
const static float2 water_direction = {0.5, 1.0};
const static float specular_exponent = 256;
const static float bump_scale = 0.075;

Texture2D<float2> normal_map_texture : register(t4);
Texture2D<float3> refraction_buffer : register(t5);
Texture2D<float> depth_buffer : register(t6);

float4 discard_vs() : SV_Position
{
   return 0.0;
}

struct Vs_fade_output
{
   float fade : FADE;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_fade_output transmissive_pass_fade_vs(Vertex_input input)
{
   Vs_fade_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fade = positionPS.w * fade_constant.z + fade_constant.w;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_lowquality_output
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD1;
   float specular : SPECULAR;
   float fade : FADE;
   float fog : FOG;

   float4 positionPS : SV_Position;
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
   output.specular = pow(HdotN, light_direction.w);

   output.diffuse_texcoords = transformer.texcoords(x_texcoords_transform[0], 
                                                    y_texcoords_transform[0]);
   output.spec_texcoords[0] = transformer.texcoords(x_texcoords_transform[1], 
                                                    y_texcoords_transform[1]);
   output.spec_texcoords[1] = transformer.texcoords(x_texcoords_transform[2], 
                                                    y_texcoords_transform[2]);

   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.fade = positionPS.w * fade_constant.z + fade_constant.w;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_normal_map_output
{
   float3 viewWS : VIEWWS;
   float fog : FOG;
   float2 texcoords[4] : TEXCOORD0;
   float fade : FADE;

   float4 positionPS : SV_Position;
};

Vs_normal_map_output normal_map_vs(Vertex_input input)
{
   Vs_normal_map_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;
   output.viewWS = view_positionWS - positionWS;

   float2 texcoords = positionWS.xz;

   const float2x2 rotation = {{cos(0.785398), -sin(0.785398)}, {sin(0.785398), cos(0.785398)}};

   output.texcoords[0] = (texcoords * tex_scales[0]) + (time_scales[0] * time);
   output.texcoords[1] = (texcoords * tex_scales[1]) * float2(0.5, 1.0) + (time_scales[1] * time);
   output.texcoords[2] = (texcoords * tex_scales[2]) * float2(0.125, 1.0) + (time_scales[2] * time);
   output.texcoords[3] = mul(rotation, texcoords * tex_scales[3]) + (time_scales[3] * time);

   output.fade = positionPS.w * fade_constant.z + fade_constant.w;
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

// Pixel Shaders //


float calc_fresnel(float normal_view_dot)
{
   float fresnel = (1.0 - normal_view_dot);
   fresnel *= fresnel;

   return lerp(fresnel_min_max.z, fresnel_min_max.w, fresnel);
}

struct Ps_fade_input
{
   float fade : FADE;
   float fog : FOG;
};

float4 transmissive_pass_fade_ps(Ps_fade_input input) : SV_Target0
{
   return float4(apply_fog(refraction_colour.rgb * input.fade, input.fog), saturate(input.fade));
}

struct Ps_normal_map_input
{
   float3 viewWS : VIEWWS;
   float fog : FOG;
   float2 texcoords[4] : TEXCOORD0;
   float fade : FADE;

   float4 positionSS : SV_Position;
};

float3 sample_normal_map(float2 texcoords)
{
   float3 normal;
   
   normal.xy = normal_map_texture.Sample(aniso_wrap_sampler, texcoords).xy;
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

float3 sample_refraction(float depth, float2 texcoords, float2 refraction_texcoords)
{
   const float4 scene_depth = depth_buffer.Gather(linear_mirror_sampler, refraction_texcoords);
   const float2 coords = (all(scene_depth > depth)) ? refraction_texcoords : texcoords;

   return refraction_buffer.SampleLevel(linear_mirror_sampler, coords, 0);
}

float4 normal_map_distorted_reflection_ps(Ps_normal_map_input input,
                                          Texture2D<float3> reflection_buffer : register(t3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);

   const float2 base_scene_texcoords = input.positionSS.xy * rt_resolution.zw;
   const float2 reflection_coords = base_scene_texcoords + bump;

   const float3 reflection = 
      reflection_buffer.SampleLevel(linear_mirror_sampler, reflection_coords, 0);
   const float3 refraction = 
      sample_refraction(input.positionSS.z, base_scene_texcoords, reflection_coords);

   const float3 viewWS = normalize(input.viewWS);
   const float NdotV = saturate(dot(normalWS, viewWS));

   const float fresnel = calc_fresnel(NdotV);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));

   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}

float4 normal_map_distorted_reflection_specular_ps(Ps_normal_map_input input,
                                                   Texture2D<float3> reflection_buffer : register(t3)) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);
   
   const float2 base_scene_texcoords = input.positionSS.xy * rt_resolution.zw;
   const float2 reflection_coords = base_scene_texcoords + bump;

   const float3 reflection =
      reflection_buffer.SampleLevel(linear_mirror_sampler, reflection_coords, 0);
   const float3 refraction = 
      sample_refraction(input.positionSS.z, base_scene_texcoords, reflection_coords);
   
   const float3 viewWS = normalize(input.viewWS);
   const float NdotV = saturate(dot(normalWS, viewWS));

   const float fresnel = calc_fresnel(NdotV);

   const float3 water_color =
      ((refraction_colour.rgb * water_fade) * refraction) + (refraction * (1.0 - water_fade));
   float3 color = lerp(water_color, reflection * reflection_colour.a, fresnel * water_fade);

   const float3 halfWS = normalize(light_direction.xyz + viewWS);
   const float NdotH = saturate(dot(normalWS, halfWS));
   color += pow(NdotH, specular_exponent);

   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade));
}

float4 normal_map_ps(Ps_normal_map_input input) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);
   
   const float3 viewWS = normalize(input.viewWS);
   const float NdotV = saturate(dot(normalWS, viewWS));

   const float fresnel = calc_fresnel(NdotV);

   const float3 color = apply_fog(refraction_colour.rgb, input.fog);

   return float4(color, saturate(input.fade) * fresnel);
}

float4 normal_map_specular_ps(Ps_normal_map_input input) : SV_Target0
{
   float2 bump;
   float3 normalWS;

   sample_normal_maps(input.texcoords, bump, normalWS);
   
   const float3 viewWS = normalize(input.viewWS);
   const float NdotV = saturate(dot(normalWS, viewWS));

   const float fresnel = calc_fresnel(NdotV);

   const float3 halfWS = normalize(light_direction.xyz + viewWS);
   const float NdotH = saturate(dot(normalWS, halfWS));
   const float spec = pow(NdotH, specular_exponent);

   float3 color = reflection_colour.rgb + spec;
   color = apply_fog(color, input.fog);

   return float4(color, saturate(input.fade) * fresnel);
}

struct Ps_lowquality_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 spec_texcoords[2] : TEXCOORD1;
   float specular : SPECULAR;
   float fade : FADE;
   float fog : FOG;
};

float4 lowquality_ps(Ps_lowquality_input input,
                     Texture2D<float4> diffuse_map_texture : register(t1)) : SV_Target0
{
   const float4 diffuse_color = 
      diffuse_map_texture.Sample(aniso_wrap_sampler, input.diffuse_texcoords);

   float4 color = refraction_colour * diffuse_color;
   color.rgb *= lighting_scale;
   color.a *= saturate(input.fade);

   color.rgb = apply_fog(color.rgb, input.fog);

   return color;
}

float4 lowquality_specular_ps(Ps_lowquality_input input,
                              Texture2D<float4> diffuse_map_texture : register(t1),
                              Texture2D<float3> specular_mask_textures[2] : register(t2)) : SV_Target0
{
   const float4 diffuse_color =
      diffuse_map_texture.Sample(aniso_wrap_sampler, input.diffuse_texcoords);

   const float3 spec_mask_0 = 
      specular_mask_textures[0].Sample(aniso_wrap_sampler, input.spec_texcoords[0]);
   const float3 spec_mask_1 =
      specular_mask_textures[1].Sample(aniso_wrap_sampler, input.spec_texcoords[1]);
   const float3 spec_mask = lerp(spec_mask_0, spec_mask_1, blend_specular_constant.rgb);

   float4 color = refraction_colour * diffuse_color;

   color.rgb += (spec_mask * input.specular);
   color.rgb *= lighting_scale;
   color.a *= saturate(input.fade);

   color.rgb = apply_fog(color.rgb, input.fog);

   return color;
}

void discard_ps()
{
   discard;
}

// OIT Entrypoints

[earlydepthstencil]
void oit_transmissive_pass_fade_ps(Ps_fade_input input, float4 positionSS : SV_Position)
{
   const float4 color = transmissive_pass_fade_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z,  color);
}

[earlydepthstencil]
void oit_normal_map_distorted_reflection_ps(Ps_normal_map_input input,
                                            Texture2D<float3> reflection_buffer : register(t3))
{
   const float4 color = 
      normal_map_distorted_reflection_ps(input, reflection_buffer);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}

[earlydepthstencil]
void oit_normal_map_distorted_reflection_specular_ps(Ps_normal_map_input input,
                                                     Texture2D<float3> reflection_buffer : register(t3)) 
{
   const float4 color = 
      normal_map_distorted_reflection_specular_ps(input, reflection_buffer);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}

[earlydepthstencil]
void oit_normal_map_ps(Ps_normal_map_input input)
{
   const float4 color = normal_map_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}

[earlydepthstencil]
void oit_normal_map_specular_ps(Ps_normal_map_input input)
{
   const float4 color = normal_map_specular_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, color);
}

[earlydepthstencil]
void oit_lowquality_ps(Ps_lowquality_input input, float4 positionSS : SV_Position,
                       Texture2D<float4> diffuse_map_texture : register(t1))
{
   const float4 color = lowquality_ps(input, diffuse_map_texture);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color);
}

[earlydepthstencil]
void oit_lowquality_specular_ps(Ps_lowquality_input input, float4 positionSS : SV_Position,
                                Texture2D<float4> diffuse_map_texture : register(t1),
                                Texture2D<float3> specular_mask_textures[2] : register(t2))
{
   const float4 color =
      lowquality_specular_ps(input, diffuse_map_texture, specular_mask_textures);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, color);
}
