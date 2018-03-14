
#include "constants_list.hlsl"
#include "ext_constants_list.hlsl"
#include "fog_utilities.hlsl"
#include "vertex_utilities.hlsl"
#include "transform_utilities.hlsl"
#include "lighting_utilities.hlsl"
#include "pixel_utilities.hlsl"

sampler2D diffuse_map : register(ps, s[0]);
sampler2D detail_map : register(ps, s[1]);
sampler2D projected_texture : register(ps, s[2]);
sampler2D shadow_map : register(ps, s[3]);

float4 blend_constant : register(ps, c[0]);
float4 shadow_blend : register(ps, c[1]);

float2 lighting_factor : register(c[CUSTOM_CONST_MIN]);
float4 texture_transforms[4] : register(vs, c[CUSTOM_CONST_MIN + 1]);

struct Vs_input
{
   float4 position : POSITION;
   float3 normals : NORMAL;
   uint4 blend_indices : BLENDINDICES;
   float4 weights : BLENDWEIGHT;
   float4 texcoords : TEXCOORD;
   float4 color : COLOR;
};

struct Vs_output_unlit
{
   float4 position : POSITION;
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 color : COLOR;
   float fog_eye_distance : DEPTH;
};

Vs_output_unlit unlit_opaque_vs(Vs_input input)
{
   Vs_output_unlit output;
    
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texture_transforms[0],
                                                             texture_transforms[1]);
   
   output.detail_texcoords = decompress_transform_texcoords(input.texcoords,
                                                            texture_transforms[2],
                                                            texture_transforms[3]);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   float4 material_color = get_material_color(input.color);

   output.color.rgb = hdr_info.z * lighting_factor.x + lighting_factor.y;
   output.color.rgb *= material_color.rgb;
   output.color.a = near_scene.fade * 0.25 + 0.5;

   return output;
}

Vs_output_unlit unlit_transparent_vs(Vs_input input)
{
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);

   Vs_output_unlit output = unlit_opaque_vs(input);
    
   Near_scene near_scene = calculate_near_scene_fade(world_position);
   near_scene = clamp_near_scene_fade(near_scene);

   output.color.a = get_material_color(input.color).a * near_scene.fade;

   return output;
}

struct Vs_output
{
   float4 position : POSITION;
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 color : COLOR0;
   float3 precalculated_light : COLOR1;

   float fog_eye_distance : DEPTH;
};

Vs_output near_opaque_vs(Vs_input input)
{
   Vs_output output;
    
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 normal = transform::normals(input.normals, input.blend_indices,
                                      input.weights);
   float3 world_normal = normals_to_world(normal);

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texture_transforms[0],
                                                             texture_transforms[1]);

   output.detail_texcoords = decompress_transform_texcoords(input.texcoords,
                                                            texture_transforms[2],
                                                            texture_transforms[3]);
   
   Near_scene near_scene = calculate_near_scene_fade(world_position);

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   output.precalculated_light = lighting.diffuse.rgb;
   output.color.rgb = get_material_color(input.color).rgb;
   output.color.a = near_scene.fade * 0.25 + 0.5;

   return output;
}

struct Vs_full_output
{
   float4 position : POSITION;

   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;

   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 color : COLOR0;
   float3 precalculated_light : COLOR1;
   float3 projection_color : COLOR2;

   float fog_eye_distance : DEPTH;
};

Vs_full_output near_opaque_shadow_projectedtex_vs(Vs_input input)
{
   Vs_full_output output;
    
   float4 world_position = transform::position(input.position, input.blend_indices,
                                               input.weights);
   float3 normal = transform::normals(input.normals, input.blend_indices,
                                      input.weights);
   float3 world_normal = normals_to_world(normal);

   output.world_position = world_position.xyz;
   output.world_normal = world_normal;
   output.position = position_project(world_position);
   output.fog_eye_distance = fog::get_eye_distance(world_position.xyz);

   output.diffuse_texcoords = decompress_transform_texcoords(input.texcoords,
                                                             texture_transforms[0],
                                                             texture_transforms[1]);

   output.detail_texcoords = decompress_transform_texcoords(input.texcoords,
                                                           texture_transforms[2],
                                                           texture_transforms[3]);

   output.projection_texcoords = mul(world_position, light_proj_matrix);

   output.shadow_texcoords = transform_shadowmap_coords(world_position);

   Near_scene near_scene = calculate_near_scene_fade(world_position);

   Lighting lighting = light::vertex_precalculate(world_normal, world_position.xyz,
                                                  get_static_diffuse_color(input.color));

   float3 material_color = get_material_color(input.color).rgb;

   float3 projection_color;
   projection_color = lighting.static_diffuse.a * material_color;
   projection_color *= hdr_info.z;
   projection_color *= light_proj_color.rgb;
   output.projection_color = projection_color;

   output.precalculated_light = lighting.diffuse.rgb;
   output.color.rgb = material_color;
   output.color.a = near_scene.fade * 0.25 + 0.5;

   return output;
}

Vs_full_output near_transparent_shadow_projectedtex_vs(Vs_input input)
{
   Vs_full_output output = near_opaque_shadow_projectedtex_vs(input);
    
   Near_scene near_scene = calculate_near_scene_fade(float4(output.world_position, 1.0));

   near_scene = clamp_near_scene_fade(near_scene);
   // near_scene.fade *= near_scene.fade;

   float4 material_color = get_material_color(input.color);

   output.color.a = near_scene.fade * material_color.a;

   return output;
}

struct Ps_input_unlit
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 color : COLOR0;
   float fog_eye_distance : DEPTH;
};

float4 unlit_opaque_ps(Ps_input_unlit input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 detail_color = tex2D(detail_map, input.detail_texcoords);

   float4 color = diffuse_color * input.color;
   color = (color * detail_color) * 2.0;

   float blend_factor = lerp(blend_constant.b, diffuse_color.a, blend_constant.a);

   float4 blended_color = color * blend_factor + color;
   color = blended_color * blend_factor + color;

   color.a = (input.color.a - 0.5) * 4.0;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 unlit_opaque_hardedged_ps(Ps_input_unlit input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 detail_color = tex2D(detail_map, input.detail_texcoords);

   float4 color = diffuse_color * input.color;
   color = (color * detail_color) * 2.0;

   float4 blended_color = color * blend_constant.a + color;
   color = blended_color * blend_constant.a + color;

   if (diffuse_color.a > 0.5) color.a = input.color.a - 0.5;
   else color.a = 0.0;

   color.a *= 4.0;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 unlit_transparent_ps(Ps_input_unlit input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 detail_color = tex2D(detail_map, input.detail_texcoords);

   float4 color = diffuse_color * input.color;
   color.rgb = (color * detail_color).rgb * 2.0;

   float4 blended_color = color * blend_constant.a + color;
   color.rgb = (blended_color * blend_constant.a + color).rgb;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   color.a = saturate(color.a);

   return color;
}

float4 unlit_transparent_hardedged_ps(Ps_input_unlit input) : COLOR
{
   float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   float4 detail_color = tex2D(detail_map, input.detail_texcoords);

   float4 color;

   float alpha = diffuse_color.a * input.color.a;

   if (diffuse_color.a > 0.5) color.a = saturate(alpha);
   else color.a = 0.0;

   color.rgb = (diffuse_color * input.color).rgb;
   color.rgb = (color * detail_color).rgb * 2.0;

   float3 blended_color;

   blended_color = color.rgb * blend_constant.a + color.rgb;
   color.rgb = blended_color * blend_constant.a + color.rgb;

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

struct Ps_input
{
   float2 diffuse_texcoords : TEXCOORD0;
   float2 detail_texcoords : TEXCOORD1;
   float4 projection_texcoords : TEXCOORD2;
   float4 shadow_texcoords : TEXCOORD3;
   float3 world_position : TEXCOORD4;
   float3 world_normal : TEXCOORD5;

   float4 color : COLOR0;
   float3 precalculated_light : COLOR1;
   float3 projection_color : COLOR2;

   float fog_eye_distance : DEPTH;
};

struct State
{
   bool projected_tex;
   bool shadowed;
   bool hardedged;
};

float4 main_opaque_ps(Ps_input input, const State state) : COLOR
{
   const float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);
   const float3 detail_color = tex2D(detail_map, input.detail_texcoords).rgb;
   const float3 projected_color = sample_projected_light(projected_texture, 
                                                         input.projection_texcoords);
   const float shadow_map_color = tex2Dproj(shadow_map, input.shadow_texcoords).a;

   // Calculate lighting.
   Pixel_lighting light = light::pixel_calculate(normalize(input.world_normal), input.world_position,
                                                 input.precalculated_light.rgb);
   light.color = light.color * lighting_factor.x + lighting_factor.y;
   light.color *= input.color.rgb;

   float4 color = 0.0;

   // Calculate projected texture lighting.
   if (state.projected_tex && state.shadowed) {
      float projection_shadow = lerp(1.0, shadow_map_color, shadow_blend.a);
      
      color.rgb = projected_color * input.projection_color * projection_shadow;
   }
   else if (state.projected_tex) {
      color.rgb = projected_color * input.projection_color;
   }

   // Apply lighting to diffuse colour.
   color.rgb += light.color;
   color.rgb *= diffuse_color.rgb; 

   if (state.shadowed) {
      float shadow_value = light.intensity * (1 - shadow_map_color);

      color.rgb *= (1 - shadow_value);
   }

   // Apply blend in detail texture.
   color.rgb = (color.rgb * detail_color) * 2.0;

   if (state.hardedged) {
      if (diffuse_color.a > 0.5) color.a = input.color.a - 0.5;
      else color.a = -0.01;

      color.a *= 4.0;
   }
   else {
      color.a = (input.color.a - 0.5) * 4.0;
   }

   color.a = saturate(color.a);

   color.rgb = fog::apply(color.rgb, input.fog_eye_distance);

   return color;
}

float4 main_transparent_ps(Ps_input input, const State state) : COLOR
{
   const float4 diffuse_color = tex2D(diffuse_map, input.diffuse_texcoords);

   float4 color = main_opaque_ps(input, state);

   if (state.hardedged) {
      if (diffuse_color.a > 0.5) color.a = (diffuse_color.a * input.color.a);
      else color.a = 0.0;
   }
   else {
      color.a = lerp(1.0, diffuse_color.a, blend_constant.b);
      color.a *= input.color.a;
   }

   color.a = saturate(color.a);

   return color;
}
