#include "adaptive_oit.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

const static float distort_blend = ps_custom_constants[0].a;

struct Vs_nodistortion_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 texcoords : TEXCOORD0;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_nodistortion_output far_vs(Vertex_input input)
{
   Vs_nodistortion_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();

   const float4 x_texcoords_transform = custom_constants[0];
   const float4 y_texcoords_transform = custom_constants[1];

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);

   output.material_color_fade = get_material_color(input.color());
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

Vs_nodistortion_output nodistortion_vs(Vertex_input input)
{
   Vs_nodistortion_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();

   const float4 x_texcoords_transform = custom_constants[0];
   const float4 y_texcoords_transform = custom_constants[1];

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *= calculate_near_fade_transparent(positionPS);
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_distortion_output
{
   float3 positionWS : POSITIONWS;
   float  scene_texcoords_x : SCENETEXCOORDSX;
   float3 normalWS : NORMALWS;
   float  scene_texcoords_y : SCENETEXCOORDSY;

   float2 bump_texcoords : TEXCOORD0;
   float3 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

float3 distortion_texcoords(float3 normalOS, float3 positionOS, 
                            float4 distort_constant, float4x4 distort_transform)
{
   float4 distortion = normalOS.xyzz * distort_constant.zzzw + float4(positionOS, 1.0);

   return mul(distortion, distort_transform).xyw;
}

Vs_distortion_output distortion_vs(Vertex_input input)
{
   const float4 distort_constant = custom_constants[0];
   const float4x4 distort_transform = 
      transpose(float4x4(custom_constants[1], custom_constants[2], custom_constants[3], custom_constants[4]));
   const float4 x_bump_texcoords_transform = custom_constants[5];
   const float4 y_bump_texcoords_transform = custom_constants[6];
   const float4 x_diffuse_texcoords_transform = custom_constants[7];
   const float4 y_diffuse_texcoords_transform = custom_constants[8];

   Vs_distortion_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionOS = transformer.positionOS();
   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();
   const float3 normalOS = transformer.normalOS();
   const float2 scene_texcoords = (positionPS.xy / positionPS.w) * float2(0.5, -0.5) + 0.5;

   output.positionWS = positionWS;
   output.positionPS = positionPS;
   output.normalWS = transformer.normalWS();

   output.scene_texcoords_x = scene_texcoords.x;
   output.scene_texcoords_y = scene_texcoords.y;
   output.distortion_texcoords = distortion_texcoords(normalOS, positionOS,
                                                      distort_constant, distort_transform);
   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix).xyww;

   output.bump_texcoords = transformer.texcoords(x_bump_texcoords_transform, 
                                                 y_bump_texcoords_transform);
   output.diffuse_texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                                    y_diffuse_texcoords_transform);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *= calculate_near_fade_transparent(positionPS);
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_nodistortion_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 texcoords : TEXCOORD0;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;
};

float4 nodistortion_ps(Ps_nodistortion_input input,
                       Texture2D<float4> diffuse_texture : register(t0)) : SV_Target0
{
   const float4 diffuse_color = 
      diffuse_texture.Sample(aniso_wrap_sampler, input.texcoords) * input.material_color_fade;
   
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting);

   float3 color = lighting.color * diffuse_color.rgb;
   color = apply_fog(color, input.fog);

   return float4(color, diffuse_color.a);
}

Texture2D<float2> bump_texture : register(t0);
Texture2D<float4> diffuse_texture : register(t2);
Texture2D<float3> projection_texture : register(t3);
Texture2D<float3> refraction_buffer : register(t4);
Texture2D<float> depth_buffer : register(t5);

struct Ps_near_input
{
   float3 positionWS : POSITIONWS;
   float  scene_texcoords_x : SCENETEXCOORDSX;
   float3 normalWS : NORMALWS;
   float  scene_texcoords_y : SCENETEXCOORDSY;

   float2 bump_texcoords : TEXCOORD0;
   float3 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionSS : SV_Position;
};

float3 sample_distort_scene(float3 distort_texcoords, float2 scene_texcoords, float depth)
{
   float2 texcoords = distort_texcoords.xy / distort_texcoords.z;
   
   const float4 scene_depth = depth_buffer.Gather(linear_mirror_sampler, texcoords);
   texcoords = (all(scene_depth > depth)) ? texcoords : scene_texcoords;

   return refraction_buffer.SampleLevel(linear_mirror_sampler, texcoords, 0);
}

float3 sample_distort_scene(float3 distort_texcoords, float2 bump, float2 scene_texcoords, float depth)
{
   const static float2x2 bump_transform = {0.002f, -0.015f, 0.015f, 0.002f};

   float2 texcoords = distort_texcoords.xy / distort_texcoords.z;
   texcoords += mul(bump, bump_transform);
   
   const float4 scene_depth = depth_buffer.Gather(linear_mirror_sampler, texcoords);
   texcoords = (all(scene_depth > depth)) ? texcoords : scene_texcoords;

   return refraction_buffer.Sample(linear_mirror_sampler, texcoords, 0);
}

float4 near_diffuse_ps(Ps_near_input input) : SV_Target0
{
   const float2 projection_texcoords = 
      input.projection_texcoords.xy / input.projection_texcoords.w;
   const float3 projection_texture_color = 
      projection_texture.Sample(linear_wrap_sampler, projection_texcoords);

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   const float2 scene_texcoords = float2(input.scene_texcoords_x, input.scene_texcoords_y);
   const float3 distortion_color = 
      sample_distort_scene(input.distortion_texcoords, scene_texcoords, input.positionSS.z);

   const float4 diffuse_color = diffuse_texture.Sample(aniso_wrap_sampler,
                                                       input.diffuse_texcoords);

   float3 color = (diffuse_color.rgb * input.material_color_fade.rgb) * lighting.color;

   const float blend = -diffuse_color.a * distort_blend + diffuse_color.a;
   color = lerp(distortion_color, color, blend);

   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}

float4 near_ps(Ps_near_input input) : SV_Target0
{
   const float2 projection_texcoords = 
      input.projection_texcoords.xy / input.projection_texcoords.w;
   const float3 projection_texture_color = 
      projection_texture.Sample(linear_wrap_sampler, projection_texcoords);

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);
   
   const float2 scene_texcoords = float2(input.scene_texcoords_x, input.scene_texcoords_y);
   const float3 distortion_color = 
      sample_distort_scene(input.distortion_texcoords, scene_texcoords, input.positionSS.z);

   float3 color = input.material_color_fade.rgb * lighting.color;

   color = lerp(distortion_color, color, distort_blend);
   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}

float4 near_diffuse_bump_ps(Ps_near_input input) : SV_Target0
{
   const float2 projection_texcoords = 
      input.projection_texcoords.xy / input.projection_texcoords.w;
   const float3 projection_texture_color = 
      projection_texture.Sample(linear_wrap_sampler, projection_texcoords);

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   const float4 diffuse_color = diffuse_texture.Sample(aniso_wrap_sampler,
                                                       input.diffuse_texcoords);

   float3 color = (diffuse_color.rgb * input.material_color_fade.rgb) * lighting.color;

   const float2 bump = bump_texture.Sample(aniso_wrap_sampler, input.bump_texcoords);
   const float2 scene_texcoords = float2(input.scene_texcoords_x, input.scene_texcoords_y);
   const float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump, 
                                                        scene_texcoords, input.positionSS.z);

   float blend = -diffuse_color.a * distort_blend + diffuse_color.a;
   color = lerp(distortion_color, color, blend);

   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}

float4 near_bump_ps(Ps_near_input input) : SV_Target0
{
   const float2 projection_texcoords = 
      input.projection_texcoords.xy / input.projection_texcoords.w;
   const float3 projection_texture_color = 
      projection_texture.Sample(linear_wrap_sampler, projection_texcoords);

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   float3 color = input.material_color_fade.rgb * lighting.color;

   const float2 bump = bump_texture.Sample(aniso_wrap_sampler, input.bump_texcoords);
   const float2 scene_texcoords = float2(input.scene_texcoords_x, input.scene_texcoords_y);
   const float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump, 
                                                        scene_texcoords, input.positionSS.z);

   color = lerp(distortion_color, color, distort_blend);
   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}

[earlydepthstencil]
void oit_nodistortion_ps(Ps_nodistortion_input input, 
                         float4 positionSS : SV_Position,
                         uint coverage : SV_Coverage,
                         Texture2D<float4> diffuse_texture : register(t0))
{
   const float4 color = nodistortion_ps(input, diffuse_texture);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, coverage, color);
}

[earlydepthstencil]
void oit_near_diffuse_ps(Ps_near_input input, uint coverage : SV_Coverage)
{
   const float4 color = near_diffuse_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}

[earlydepthstencil]
void oit_near_ps(Ps_near_input input, uint coverage : SV_Coverage)
{
   const float4 color = near_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}

[earlydepthstencil]
void oit_near_diffuse_bump_ps(Ps_near_input input, uint coverage : SV_Coverage)
{
   const float4 color = near_diffuse_bump_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}

[earlydepthstencil]
void oit_near_bump_ps(Ps_near_input input, uint coverage : SV_Coverage)
{
   const float4 color = near_bump_ps(input);

   aoit::write_pixel((uint2)input.positionSS.xy, input.positionSS.z, coverage, color);
}
