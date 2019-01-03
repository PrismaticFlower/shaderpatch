
#include "generic_vertex_input.hlsl"
#include "vertex_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

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

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();

   const float4 x_texcoords_transform = custom_constants[0];
   const float4 y_texcoords_transform = custom_constants[1];

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);

   output.material_color_fade = get_material_color(input.color());
   output.static_lighting = get_static_diffuse_color(input.color());

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);

   return output;
}

Vs_nodistortion_output nodistortion_vs(Vertex_input input)
{
   Vs_nodistortion_output output;

   Transformer transformer = create_transformer(input);

   float3 positionWS = transformer.positionWS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();

   const float4 x_texcoords_transform = custom_constants[0];
   const float4 y_texcoords_transform = custom_constants[1];

   output.texcoords = transformer.texcoords(x_texcoords_transform, y_texcoords_transform);

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);
   near_fade = saturate(near_fade);
   near_fade *= near_fade;

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *= near_fade;
   output.static_lighting = get_static_diffuse_color(input.color());

   return output;
}

struct Vs_distortion_output
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

float4 distortion_texcoords(float3 normalOS, float3 positionOS, 
                            float4 distort_constant, float4x4 distort_transform)
{
   float4 distortion = normalOS.xyzz * distort_constant.zzzw + float4(positionOS, 1.0);

   return mul(distortion, distort_transform).xyww;
}

Vs_distortion_output distortion_vs(Vertex_input input)
{
   const float4 distort_constant = custom_constants[0];
   const float4x4 distort_transform = 
      {custom_constants[1], custom_constants[2], custom_constants[3], custom_constants[4]};
   const float4 x_bump_texcoords_transform = custom_constants[5];
   const float4 y_bump_texcoords_transform = custom_constants[6];
   const float4 x_diffuse_texcoords_transform = custom_constants[7];
   const float4 y_diffuse_texcoords_transform = custom_constants[8];

   Vs_distortion_output output;

   Transformer transformer = create_transformer(input);

   float3 positionOS = transformer.positionOS();
   float3 positionWS = transformer.positionWS();
   float3 normalOS = transformer.normalOS();

   output.positionPS = transformer.positionPS();
   output.positionWS = positionWS;
   output.normalWS = transformer.normalWS();

   output.distortion_texcoords = distortion_texcoords(normalOS, positionOS,
                                                      distort_constant, distort_transform);
   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix).xyww;

   output.bump_texcoords = transformer.texcoords(x_bump_texcoords_transform, 
                                                 y_bump_texcoords_transform);
   output.diffuse_texcoords = transformer.texcoords(x_diffuse_texcoords_transform,
                                                    y_diffuse_texcoords_transform);

   float near_fade;
   calculate_near_fade_and_fog(positionWS, near_fade, output.fog);
   near_fade = saturate(near_fade);
   near_fade *= near_fade;

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *= near_fade;
   output.static_lighting = get_static_diffuse_color(input.color());

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
Texture2D<float3> refraction_buffer : register(t4);
Texture2D<float4> diffuse_texture : register(t2);
Texture2D<float3> projection_texture : register(t3);

struct Ps_near_input
{
   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float2 bump_texcoords : TEXCOORD0;
   float4 distortion_texcoords : TEXCOORD1;
   float2 diffuse_texcoords : TEXCOORD2;
   float4 projection_texcoords : TEXCOORD3;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;
};

float4 near_diffuse_ps(Ps_near_input input) : SV_Target0
{
   const float2 projection_texcoords = 
      input.projection_texcoords.xy / input.projection_texcoords.w;
   const float3 projection_texture_color = 
      projection_texture.Sample(linear_wrap_sampler, projection_texcoords);

   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, true,
                                        projection_texture_color);

   const float2 distortion_texcoords = 
      input.distortion_texcoords.xy / input.distortion_texcoords.w;
   const float3 distortion_color = 
      refraction_buffer.SampleLevel(linear_wrap_sampler, distortion_texcoords, 0);

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

   const float2 distortion_texcoords =
      input.distortion_texcoords.xy / input.distortion_texcoords.w;
   const float3 distortion_color =
      refraction_buffer.SampleLevel(linear_wrap_sampler, distortion_texcoords, 0);

   float3 color = input.material_color_fade.rgb * lighting.color;

   color = lerp(distortion_color, color, distort_blend);
   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}

float3 sample_distort_scene(float4 distort_texcoords, float2 bump)
{
   const static float2x2 bump_transform = {0.002f, -0.015f, 0.015f, 0.002f};

   float2 texcoords = distort_texcoords.xy / distort_texcoords.w;
   texcoords += mul(bump, bump_transform);

   return refraction_buffer.Sample(linear_wrap_sampler, texcoords);
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
   const float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump);

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
   const float3 distortion_color = sample_distort_scene(input.distortion_texcoords, bump);

   color = lerp(distortion_color, color, distort_blend);
   color = apply_fog(color, input.fog);

   return float4(color, input.material_color_fade.a);
}
