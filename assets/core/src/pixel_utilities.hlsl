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

#define WITH_NORMALMAP_UNSIGNED
#define WITH_NORMALMAP_2CHANNEL

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

float3 perturb_normal(sampler2D samp, float2 texcoord, float3 N, float3 V)
{
   // assume N, the interpolated vertex normal and 
   // V, the view vector (vertex to eye)
   float3 map = tex2D(samp, texcoord).xyz;
#ifdef WITH_NORMALMAP_UNSIGNED
   map = map * 255. / 127. - 128. / 127.;
#endif
#ifdef WITH_NORMALMAP_2CHANNEL
   map.z = sqrt(1. - dot(map.xy, map.xy));
#endif
#ifdef WITH_NORMALMAP_GREEN_UP
   map.y = -map.y;
#endif
   float3x3 TBN = cotangent_frame(N, -V, texcoord);
   return normalize(mul(map, TBN));
}

float3 sample_projected_light(sampler2D light_texture, float4 texcoords)
{
   if (cube_map_light_projection)
   {
      return texCUBEproj(cube_light_texture, texcoords).rgb;
   }
   else
   {
      return tex2Dproj(light_texture, texcoords).rgb;
   }
}

float4 tex2Dlod(sampler2D samp, float2 texcoord, float lod)
{
   return tex2Dlod(samp, float4(texcoord, 0.0, lod));
}

float4 tex2Dgaussian(sampler2D samp, float2 texcoord, float2 texel_size)
{
   const static float centre_weight = 0.083355;
   const static float mid_weight = 0.10267899999999999;
   const static float mid_offset = 1.3446761265692109;
   const static float corner_weight = 0.126482;
   const static float2 corner_offset = { 1.3446735503866163, 1.3446735503866163 };

   float4 color = tex2Dlod(samp, texcoord, 0) * centre_weight;

   color += 
      tex2Dlod(samp, texcoord + (mid_offset * texel_size) * float2(1, 0), 0) * mid_weight;
   color += 
      tex2Dlod(samp, texcoord - (mid_offset * texel_size) * float2(1, 0), 0) * mid_weight;
   color += 
      tex2Dlod(samp, texcoord + (mid_offset * texel_size) * float2(0, 1), 0) * mid_weight;
   color += 
      tex2Dlod(samp, texcoord - (mid_offset * texel_size) * float2(0, 1), 0) * mid_weight;

   color += 
      tex2Dlod(samp, texcoord + (corner_offset * texel_size), 0) * corner_weight;
   color += 
      tex2Dlod(samp, texcoord + (corner_offset * texel_size) * float2(-1, 1), 0) * corner_weight;
   color += 
      tex2Dlod(samp, texcoord + (corner_offset * texel_size) * float2(-1, -1), 0) * corner_weight;
   color += 
      tex2Dlod(samp, texcoord + (corner_offset * texel_size) * float2(1, -1), 0) * corner_weight;

   return color;
}

float3 linear_to_srgb(float3 color) {
   return (color <= 0.0031308) ? (12.92 * color) : mad(1.055, pow(color, 1.0 / 2.4), -0.055);
}

float3 srgb_to_linear(float3 color) {
   return (color <= 0.04045) ? (color / 12.92) : pow(mad(color, 1.0 / 1.055, 0.055 / 1.055), 2.4);
}

#endif