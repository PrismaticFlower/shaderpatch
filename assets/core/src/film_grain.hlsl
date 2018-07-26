#ifndef FILMGRAIN_INCLUDED
#define FILMGRAIN_INCLUDED

#include "postprocess_common.hlsl"

namespace filmgrain
{

/*
Film Grain post-process shader v1.1
Martins Upitis (martinsh) devlog-martinsh.blogspot.com
2013

Modified by me (SleepKiller) for HLSL and SP, but largely unchanged.

--------------------------
This work is licensed under a Creative Commons Attribution 3.0 Unported License.
So you are free to share, modify and adapt it for your needs, and even use it for commercial use.
I would also love to hear about a project you are using it.

Have fun,
Martins
--------------------------

Perlin noise shader by toneburst:
http://machinesdontcare.wordpress.com/2009/06/25/3d-perlin-noise-sphere-vertex-shader-sourcecode/
*/

float4 rnm(float2 tc)
{
   float noise = sin(dot(tc + randomness.w, float2(12.9898, 78.233))) * 43758.5453;

   float R = frac(noise) * 2.0 - 1.0;
   float G = frac(noise * 1.2154) * 2.0 - 1.0;
   float B = frac(noise * 1.3453) * 2.0 - 1.0;
   float A = frac(noise * 1.3647) * 2.0 - 1.0;

   return float4(R, G, B, A);
}

float fade(float t)
{
   return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float pnoise3D(float3 p)
{
   const static float perm_tex_unit = 1.0 / 256.0;       // Perm texture texel-size
   const static float perm_tex_unit_half = 0.5 / 256.0;  // Half perm texture texel-size

   float3 pi = perm_tex_unit * floor(p) + perm_tex_unit_half; // Integer part, scaled so +1 moves permTexUnit texel
   // and offset 1/2 texel to sample texel centers
   float3 pf = frac(p);     // Fractional part for interpolation

   // Noise contributions from (x=0, y=0), z=0 and z=1
   float  perm00 = rnm(pi.xy).a;
   float3 grad000 = rnm(float2(perm00, pi.z)).rgb * 4.0 - 1.0;
   float  n000 = dot(grad000, pf);
   float3 grad001 = rnm(float2(perm00, pi.z + perm_tex_unit)).rgb * 4.0 - 1.0;
   float  n001 = dot(grad001, pf - float3(0.0, 0.0, 1.0));

   // Noise contributions from (x=0, y=1), z=0 and z=1
   float  perm01 = rnm(pi.xy + float2(0.0, perm_tex_unit)).a;
   float3 grad010 = rnm(float2(perm01, pi.z)).rgb * 4.0 - 1.0;
   float  n010 = dot(grad010, pf - float3(0.0, 1.0, 0.0));
   float3 grad011 = rnm(float2(perm01, pi.z + perm_tex_unit)).rgb * 4.0 - 1.0;
   float  n011 = dot(grad011, pf - float3(0.0, 1.0, 1.0));

   // Noise contributions from (x=1, y=0), z=0 and z=1
   float  perm10 = rnm(pi.xy + float2(perm_tex_unit, 0.0)).a;
   float3 grad100 = rnm(float2(perm10, pi.z)).rgb * 4.0 - 1.0;
   float  n100 = dot(grad100, pf - float3(1.0, 0.0, 0.0));
   float3 grad101 = rnm(float2(perm10, pi.z + perm_tex_unit)).rgb * 4.0 - 1.0;
   float  n101 = dot(grad101, pf - float3(1.0, 0.0, 1.0));

   // Noise contributions from (x=1, y=1), z=0 and z=1
   float  perm11 = rnm(pi.xy + perm_tex_unit).a;
   float3 grad110 = rnm(float2(perm11, pi.z)).rgb * 4.0 - 1.0;
   float  n110 = dot(grad110, pf - float3(1.0, 1.0, 0.0));
   float3 grad111 = rnm(float2(perm11, pi.z + perm_tex_unit)).rgb * 4.0 - 1.0;
   float  n111 = dot(grad111, pf - float3(1.0, 1.0, 1.0));

   // Blend contributions along x
   float4 n_x = lerp(float4(n000, n001, n010, n011), float4(n100, n101, n110, n111), fade(pf.x));

   // Blend contributions along y
   float2 n_xy = lerp(n_x.xy, n_x.zw, fade(pf.y));

   // Blend contributions along z
   float n_xyz = lerp(n_xy.x, n_xy.y, fade(pf.z));

   // We're done, return the final noise value.
   return n_xyz; 
}

//2d coordinate orientation thing
float2 coord_rot(float2 tc, float angle)
{
   float aspect = scene_resolution.x / scene_resolution.y;
   float rotX = 
      ((tc.x * 2.0 - 1.0) * aspect * cos(angle)) - ((tc.y * 2.0 - 1.0) * sin(angle));
   float rotY = 
      ((tc.y * 2.0 - 1.0) * cos(angle)) + ((tc.x * 2.0 - 1.0) * aspect * sin(angle));
   rotX = ((rotX / aspect) * 0.5 + 0.5);
   rotY = rotY * 0.5 + 0.5;

   return float2(rotX, rotY);
}

void apply(float2 texcoords, inout float3 color)
{
   const static float3 rot_offset = {1.425, 3.892, 5.835}; // rotation offset values	
   float2 rot_coords_r = coord_rot(texcoords, randomness.r + rot_offset.x);
   
   const float2 grainsize = scene_resolution / film_grain_size;
   
   float3 noise = pnoise3D(float3(rot_coords_r *  grainsize, 0.0));

   if (film_grain_colored) {
      float2 rot_coords_g = coord_rot(texcoords, randomness.g + rot_offset.y);
      float2 rot_coords_b = coord_rot(texcoords, randomness.b + rot_offset.z);
      noise.g = 
         lerp(noise.r, pnoise3D(float3(rot_coords_g * grainsize, 1.0)), film_grain_color_amount);
      noise.b =
         lerp(noise.r, pnoise3D(float3(rot_coords_b * grainsize, 2.0)), film_grain_color_amount);
   }

   //noisiness response curve based on scene luminance
   float3 lumcoeff = {0.299, 0.587, 0.114};
   float luminance = lerp(0.0, dot(color, lumcoeff), film_grain_luma_amount);
   float lum = smoothstep(0.2, 0.0, luminance);
   lum += luminance;

   noise = lerp(noise, 0.0, pow(lum, 4.0));
   color = color + noise * film_grain_amount;
}

}

#endif