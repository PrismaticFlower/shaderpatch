#ifndef PIXEL_UTILS_INCLUDED
#define PIXEL_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

TextureCube<float3> cube_projected_texture : register(t4);

float3x3 generate_tangent_to_world(const float3 normalWS, const float3 positionWS, const float2 texcoords)
{
   const float3 pos_dx = ddx(positionWS);
   const float3 pos_dy = ddy(positionWS);
   const float2 tex_dx = ddx(texcoords);
   const float2 tex_dy = ddy(texcoords);
   const float3 unorm_tangentWS = (tex_dy.y * pos_dx - tex_dx.y * pos_dy) * rcp(tex_dx.x * tex_dy.y - tex_dy.x * tex_dx.y);
   
   const float3 tangentWS = normalize(unorm_tangentWS - normalWS * dot(normalWS, unorm_tangentWS));
   const float3 bitangentWS = cross(normalWS, tangentWS);

   return float3x3(tangentWS, bitangentWS, normalWS);
}

float3 sample_normal_map(Texture2D<float2> tex, SamplerState samp, float2 texcoords)
{
   float3 normal;
   normal.xy = tex.Sample(samp, texcoords) * (255.0 / 127.0) - (128.0 / 127.0);
   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   return normal;
}

float3 sample_normal_map(Texture2D<float2> tex, SamplerState samp, float2 texcoords, float lod)
{
   float3 normal;
   normal.xy = tex.SampleLevel(samp, texcoords, lod) * (255.0 / 127.0) - (128.0 / 127.0);
   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   return normal;
}

float3 sample_normal_map_gloss(Texture2D<float4> tex, SamplerState samp, float2 texcoords, out float gloss)
{
   float4 normal_gloss = tex.Sample(samp, texcoords);
   float3 normal = normal_gloss.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   gloss = normal_gloss.w;

   return normal;
}

float3 sample_normal_map_gloss(Texture2D<float4> tex, SamplerState samp, float mip_level, 
                               float2 texcoords, out float gloss)
{
   float4 normal_gloss = tex.SampleLevel(samp, texcoords, mip_level);
   float3 normal = normal_gloss.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   gloss = normal_gloss.w;

   return normal;
}

float3 sample_normal_map_gloss(Texture2D<float4> tex, Texture2D<float2> detail_tex, SamplerState samp, 
                               float2 texcoords, float2 detail_texcoords, out float gloss)
{
   float4 normal_gloss = tex.Sample(samp, texcoords);
   float3 normal = normal_gloss.xyz * (255.0 / 127.0) - (128.0 / 127.0);

   normal.xy *= (detail_tex.Sample(samp, detail_texcoords) * (255.0 / 127.0) - (128.0 / 127.0));
   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   gloss = normal_gloss.w;

   return normal;
}

interface Parallax_input_texture
{
   float CalculateLevelOfDetail(SamplerState samp, float2 texcoords);

   float SampleLevel(SamplerState samp, float2 texcoords, float mip);

   float Sample(SamplerState samp, float2 texcoords);
};

float2 parallax_occlusion_map(Parallax_input_texture height_map, const float height_scale, 
                              const float2 texcoords, const float3 unorm_viewTS)
{
   const float fade_start = 64.0;
   const float fade_length = 256.0;
   const uint num_steps = 16;
   const float mip_level = height_map.CalculateLevelOfDetail(linear_wrap_sampler, texcoords);

   const float3 viewTS = normalize(unorm_viewTS);
   const float2 parallax_direction = normalize(viewTS.xy);

   const float view_length = length(viewTS);
   const float parallax_length = sqrt(view_length * view_length - viewTS.z * viewTS.z) / viewTS.z;
   const float2 parallax_offset = parallax_direction * parallax_length * height_scale;

   const float view_distance = length(unorm_viewTS);
   const float fade = 1.0 - saturate((view_distance - fade_start) / fade_length);

#pragma warning(disable : 4000)
   [branch] if (fade == 0.0) return texcoords;
#pragma warning(enable : 4000)

   const float step_size = 1.0 / (float)(num_steps * 4u);
   float curr_height = 0.0;
   float prev_height = 1.0;
   
   const float2 offset_per_step = step_size * parallax_offset;
   float2 current_offset = texcoords;
   float  current_bound = 1.0;
   
   float2 pt1 = 0;
   float2 pt2 = 0;
   
   [loop] for (uint step = 0; step < num_steps; ++step) {
      float2 step_texcoords[4];

      step_texcoords[0] = current_offset - (offset_per_step);
      step_texcoords[1] = current_offset - (offset_per_step * 2.0);
      step_texcoords[2] = current_offset - (offset_per_step * 3.0);
      step_texcoords[3] = current_offset - (offset_per_step * 4.0);

      float4 step_heights;

      step_heights[0] = height_map.SampleLevel(linear_wrap_sampler, step_texcoords[0], mip_level);
      step_heights[1] = height_map.SampleLevel(linear_wrap_sampler, step_texcoords[1], mip_level);
      step_heights[2] = height_map.SampleLevel(linear_wrap_sampler, step_texcoords[2], mip_level);
      step_heights[3] = height_map.SampleLevel(linear_wrap_sampler, step_texcoords[3], mip_level);

      float4 step_bounds;

      step_bounds[0] = current_bound - (step_size);
      step_bounds[1] = current_bound - (step_size * 2.0);
      step_bounds[2] = current_bound - (step_size * 3.0);
      step_bounds[3] = current_bound - (step_size * 4.0);

      const bool4 intersection = step_heights > step_bounds;

      [branch] if (any(intersection)) {
         float prev_bound;

         if (intersection[0]) {
            prev_bound = current_bound;
            curr_height = step_heights[0];
            current_bound = step_bounds[0];
         }
         else if (intersection[1]) {
            prev_bound = step_bounds[0];
            prev_height = step_heights[0];
            curr_height = step_heights[1];
            current_bound = step_bounds[1];
         }
         else if (intersection[2]) {
            prev_bound = step_bounds[1];
            prev_height = step_heights[1];
            curr_height = step_heights[2];
            current_bound = step_bounds[2];
         }
         else if (intersection[3]) {
            prev_bound = step_bounds[2];
            prev_height = step_heights[2];
            curr_height = step_heights[3];
            current_bound = step_bounds[3];
         }

         pt1 = float2(current_bound, curr_height);
         pt2 = float2(prev_bound, prev_height);
   
         break;
      }

      current_offset = step_texcoords[3];
      current_bound = step_bounds[3];
      prev_height = step_heights[3];
   }

   const float delta2 = pt2.x - pt2.y;
   const float delta1 = pt1.x - pt1.y;  
   const float denominator = delta2 - delta1;
   const float parallax_amount = 
      (denominator != 0.0) ? (pt1.x * delta2 - pt2.x * delta1) / denominator : 0.0;
   const float2 final_offset = parallax_offset * (1 - parallax_amount);

   return lerp(texcoords, texcoords - final_offset, fade);
}

float2 parallax_offset_map(Parallax_input_texture height_map, const float height_scale,
                           const float2 texcoords, const float3 viewTS)
{
   const float height = height_map.Sample(aniso_wrap_sampler, texcoords);

   const float2 offset = viewTS.xy * (height * height_scale);

   return texcoords - offset;
}

float3 sample_projected_light(Texture2D<float3> projected_texture, float4 texcoords)
{
   if (cube_projtex) {
      const float3 proj_texcoords = texcoords.xyz / texcoords.w;
   
      return cube_projected_texture.Sample(projtex_sampler, proj_texcoords);
   }
   else {
     const float2 proj_texcoords = texcoords.xy / texcoords.w;
   
     return projected_texture.Sample(projtex_sampler, proj_texcoords);
   }
}

float3 gaussian_sample(Texture2D<float3> tex, SamplerState samp,
                       float2 texcoords, float2 texel_size)
{
   const static float centre_weight = 0.083355;
   const static float mid_weight = 0.10267899999999999;
   const static float mid_offset = 1.3446761265692109;
   const static float corner_weight = 0.126482;
   const static float2 corner_offset = { 1.3446735503866163, 1.3446735503866163 };

   float3 color = tex.SampleLevel(samp, texcoords, 0) * centre_weight;

   color += 
      tex.SampleLevel(samp, texcoords + (mid_offset * texel_size) * float2(1, 0), 0) * mid_weight;
   color += 
      tex.SampleLevel(samp, texcoords - (mid_offset * texel_size) * float2(1, 0), 0) * mid_weight;
   color += 
      tex.SampleLevel(samp, texcoords + (mid_offset * texel_size) * float2(0, 1), 0) * mid_weight;
   color += 
      tex.SampleLevel(samp, texcoords - (mid_offset * texel_size) * float2(0, 1), 0) * mid_weight;

   color += 
      tex.SampleLevel(samp, texcoords + (corner_offset * texel_size), 0) * corner_weight;
   color += 
      tex.SampleLevel(samp, texcoords + (corner_offset * texel_size) * float2(-1, 1), 0) * corner_weight;
   color += 
      tex.SampleLevel(samp, texcoords + (corner_offset * texel_size) * float2(-1, -1), 0) * corner_weight;
   color += 
      tex.SampleLevel(samp, texcoords + (corner_offset * texel_size) * float2(1, -1), 0) * corner_weight;

   return color;
}

float3 apply_fog(float3 color, float fog)
{
   if (fog_enabled) {
      return lerp(fog_color, color, fog);
   }
   else {
      return color;
   }
}

#endif