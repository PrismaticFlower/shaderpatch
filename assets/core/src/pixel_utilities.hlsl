#ifndef PIXEL_UTILS_INCLUDED
#define PIXEL_UTILS_INCLUDED

#include "ext_constants_list.hlsl"

float3 blend_tangent_space_normals(float3 N0, float3 N1)
{
   return float3(N0.xy + N1.xy, N0.z * N1.z);
}

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
   map.z = sqrt(1. - dot(map.xy, map.xy));

#ifdef WITH_NORMALMAP_GREEN_UP
   map.y = -map.y;
#endif

   float3x3 TBN = cotangent_frame(N, -V, texcoords);
   return normalize(mul(map, TBN));
}

float3 sample_projected_light(Texture2D<float3> projected_texture, float4 texcoords)
{
   // FIXME: SM 3.0 Flow Control workaround.
   const float w_rcp = rcp(texcoords.w);

   // if (cube_map_light_projection) {
   //    const float3 proj_texcoords = texcoords.xyz * w_rcp;
   // 
   //    return cube_projected_texture.Sample(projected_texture_sampler, proj_texcoords);
   // }
   // else {
      const float2 proj_texcoords = texcoords.xy * w_rcp;

      return projected_texture.Sample(projected_texture_sampler, proj_texcoords);
   // }
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

float3 linear_to_srgb(float3 color) {
   return (color <= 0.0031308) ? (12.92 * color) : mad(1.055, pow(color, 1.0 / 2.4), -0.055);
}

float3 srgb_to_linear(float3 color) {
   return (color <= 0.04045) ? (color / 12.92) : pow(mad(color, 1.0 / 1.055, 0.055 / 1.055), 2.4);
}

#endif