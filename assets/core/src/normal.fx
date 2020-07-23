
#include "adaptive_oit.hlsl"
#include "constants_list.hlsl"
#include "generic_vertex_input.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_sampler_states.hlsl"
#include "pixel_utilities.hlsl"
#include "vertex_transformer.hlsl"
#include "vertex_utilities.hlsl"

// clang-format off

Texture2D<float4> diffuse_map : register(t0);
Texture2D<float3> detail_map : register(t1);
Texture2D<float3> projected_texture : register(t2);
Texture2D<float4> shadow_map : register(t3);

const static float4 blend_constants = ps_custom_constants[0];
const static float4 texture_transforms[4] = 
   {custom_constants[1], custom_constants[2], custom_constants[3], custom_constants[4]};
const static bool lighting_pass = custom_constants[0].x >= 1.0;


const static bool use_transparency = NORMAL_USE_TRANSPARENCY;
const static bool use_hardedged_test = NORMAL_USE_HARDEDGED_TEST;
const static bool use_shadow_map = NORMAL_USE_SHADOW_MAP;
const static bool use_projected_texture = NORMAL_USE_PROJECTED_TEXTURE;

struct Vs_output_unlit
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 material_color_fade : MATCOLOR;
   float fog : FOG;
   float4 positionPS : SV_Position;
};

Vs_output_unlit unlit_main_vs(Vertex_input input)
{
   Vs_output_unlit output;
    
   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();

   output.positionPS = positionPS;

   output.diffuse_texcoords = transformer.texcoords(texture_transforms[0],
                                                    texture_transforms[1]);  
   output.detail_texcoords = transformer.texcoords(texture_transforms[2],
                                                   texture_transforms[3]);

   output.material_color_fade.rgb = lighting_scale;
   output.material_color_fade.a = 
      use_transparency ? calculate_near_fade_transparent(positionPS) : 
                         calculate_near_fade(positionPS);
   output.material_color_fade *= get_material_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Vs_output
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   noperspective float2 shadow_texcoords : TEXCOORD3;

   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;

   float4 positionPS : SV_Position;
};

Vs_output main_vs(Vertex_input input)
{
   Vs_output output;

   Transformer transformer = create_transformer(input);

   const float3 positionWS = transformer.positionWS();
   const float4 positionPS = transformer.positionPS();
   const float3 normalWS = transformer.normalWS();

   output.positionWS = positionWS;
   output.normalWS = normalWS;
   output.positionPS = positionPS;

   output.diffuse_texcoords = transformer.texcoords(texture_transforms[0],
                                                    texture_transforms[1]);  
   output.detail_texcoords = transformer.texcoords(texture_transforms[2],
                                                   texture_transforms[3]);

   output.projection_texcoords = mul(float4(positionWS, 1.0), light_proj_matrix);
   output.shadow_texcoords = transform_shadowmap_coords(positionPS);

   output.material_color_fade = get_material_color(input.color());
   output.material_color_fade.a *=
      use_transparency ? calculate_near_fade_transparent(positionPS) : 
                         calculate_near_fade(positionPS);
   output.static_lighting = get_static_diffuse_color(input.color());
   output.fog = calculate_fog(positionWS, positionPS);

   return output;
}

struct Ps_input_unlit
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 material_color_fade : MATCOLOR;
   float fog : FOG;
};

float get_glow_factor(float4 diffuse_map_color)
{
   if (use_hardedged_test || use_transparency) return blend_constants.a;
   
   return lerp(blend_constants.b, diffuse_map_color.a, blend_constants.a);
}

float4 unlit_main_ps(Ps_input_unlit input) : SV_Target0
{
   const float4 diffuse_map_color = diffuse_map.Sample(aniso_wrap_sampler,
                                                       input.diffuse_texcoords);
   const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, input.detail_texcoords);

   float3 color = diffuse_map_color.rgb * input.material_color_fade.rgb;
   color = (color * detail_color) * 2.0;

   const float glow_factor = get_glow_factor(diffuse_map_color);
   const float3 glow_color = color * glow_factor + color;
     
   color = glow_color * glow_factor + color;

   float alpha = input.material_color_fade.a;

   if (use_transparency) {
      if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

      alpha *= diffuse_map_color.a;
   }
   else if (use_hardedged_test) {
      if (diffuse_map_color.a < 0.5) discard;
   }

   color = apply_fog(color, input.fog);

   return float4(color, saturate(alpha));
}

struct Ps_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   noperspective float2 shadow_texcoords : TEXCOORD3;

   float3 positionWS : POSITIONWS;
   float3 normalWS : NORMALWS;

   float4 material_color_fade : MATCOLOR;
   float3 static_lighting : STATICLIGHT;

   float fog : FOG;
};

float4 main_ps(Ps_input input) : SV_Target0
{
   const float4 diffuse_map_color = diffuse_map.Sample(aniso_wrap_sampler,
                                                       input.diffuse_texcoords);
   const float3 detail_color = detail_map.Sample(aniso_wrap_sampler, input.detail_texcoords);

   float3 diffuse_color = diffuse_map_color.rgb * input.material_color_fade.rgb;
   diffuse_color = diffuse_color * detail_color * 2.0;
   
   const float3 projection_texture_color = 
      use_projected_texture ? sample_projected_light(projected_texture, 
                                                     input.projection_texcoords) :
                              0.0;

   // Calculate lighting.
   Lighting lighting = light::calculate(normalize(input.normalWS), input.positionWS,
                                        input.static_lighting, use_projected_texture,
                                        projection_texture_color);

   float3 color = lighting_pass ? lighting.color : lighting_scale.xxx;

   color *= diffuse_color; 

   if (use_shadow_map) {
      const float shadow_map_value =
         use_shadow_map ? shadow_map.SampleLevel(linear_clamp_sampler, input.shadow_texcoords, 0).a
                        : 1.0;

      float shadow = 1.0 - (lighting.intensity * (1.0 - shadow_map_value));

      color = color * shadow;
   }

   float alpha;

   if (use_transparency) {
      if (use_hardedged_test) {
         if (diffuse_map_color.a < 0.5) discard;

         alpha = diffuse_map_color.a * input.material_color_fade.a;
      }
      else {
         alpha = 
            lerp(1.0, diffuse_map_color.a, blend_constants.b) * input.material_color_fade.a;
      }
   }
   else {
      if (use_hardedged_test && diffuse_map_color.a < 0.5) discard;

      alpha = input.material_color_fade.a;
   }

   color = apply_fog(color, input.fog);

   return float4(color, saturate(alpha));
}

[earlydepthstencil]
void oit_unlit_main_ps(Ps_input_unlit input, float4 positionSS : SV_Position, uint coverage : SV_Coverage) {
   const float4 color = unlit_main_ps(input);

   aoit::write_pixel(positionSS.xy, positionSS.z, coverage, color);
}

[earlydepthstencil]
void oit_main_ps(Ps_input input, float4 positionSS : SV_Position, uint coverage : SV_Coverage)
{
   const float4 color = main_ps(input);

   aoit::write_pixel((uint2)positionSS.xy, positionSS.z, coverage, color);
}