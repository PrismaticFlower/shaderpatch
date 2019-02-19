#ifndef PIXEL_UTILS_INCLUDED
#define PIXEL_UTILS_INCLUDED

#include "constants_list.hlsl"
#include "pixel_sampler_states.hlsl"

TextureCube<float3> cube_projected_texture : register(t127);

// Christian Schüler's Normal Mapping Without Precomputed Tangents code.
// Taken from http://www.thetenthplanet.de/archives/1180, be sure to go
// check out his article on it. 

float3x3 cotangent_frame(float3 N, float3 p, float2 uv)
{
   // get edge vectors of the pixel triangle
   float3 dp1 = ddx(p);
   float3 dp2 = ddy(p);
   float2 duv1 = ddx(uv);
   float2 duv2 = ddy(uv);

   // solve the linear system
   float3 dp2perp = cross(dp2, N);
   float3 dp1perp = cross(N, dp1);
   float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
   float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

   // construct a scale-invariant frame 
   float invmax = rsqrt(max(dot(T, T), dot(B, B)));
   return float3x3(T * invmax, B * invmax, N);
}

float3 perturb_normal(Texture2D<float2> tex, SamplerState samp, 
                      float2 texcoords, float3 N, float3 V)
{
   // assume N, the interpolated vertex normal and 
   // V, the view vector (vertex to eye)

   float3 map;
   map.xy = tex.Sample(samp, texcoords) * 255. / 127. - 128. / 127.;
   map.z = sqrt(1.0 - saturate(dot(map.xy, map.xy)));

   float3x3 TBN = cotangent_frame(N, -V, texcoords);
   float3 result = normalize(mul(map, TBN));

   return any(isnan(result)) ? N : result;
}

float3 sample_normal_map(Texture2D<float2> tex, SamplerState samp, float2 texcoords)
{
   float3 normal;
   normal.xy = tex.Sample(samp, texcoords) * 2.0 - 1.0;
   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   return normal;
}

float3 sample_normal_map_gloss(Texture2D<float4> tex, SamplerState samp, float2 texcoords, out float gloss)
{
   float4 normal_gloss = tex.Sample(samp, texcoords);
   float3 normal = normal_gloss.xyz * 2.0 - 1.0;

   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   gloss = normal_gloss.w;

   return normal;
}

float3 sample_normal_map_gloss(Texture2D<float4> tex, Texture2D<float2> detail_tex, SamplerState samp, 
                               float2 texcoords, float2 detail_texcoords, out float gloss)
{
   float4 normal_gloss = tex.Sample(samp, texcoords);
   float3 normal = normal_gloss.xyz * 2.0 - 1.0;

   normal.xy *= (detail_tex.Sample(samp, detail_texcoords) * 2.0 - 1.0);
   normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));

   gloss = normal_gloss.w;

   return normal;
}

float3 perturb_normal(float3 normalTS, float3 normal, float3 tangent, float bitangent_sign)
{
   const float3 bitangent = bitangent_sign * cross(normal, tangent);

   return normalize(normalTS.x * tangent + normalTS.y * bitangent + normalTS.z * normal);
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