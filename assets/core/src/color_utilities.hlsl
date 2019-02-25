#ifndef COLOR_UTILS_INCLUDED
#define COLOR_UTILS_INCLUDED

float3 srgb_to_linear(const float3 color)
{
   const float3 srgb_low = color / 12.92;
   const float3 srgb_hi = pow(abs(color) / 1.055, 2.4);

   return (color <= 0.04045) ? srgb_low : srgb_hi;
}

float3 linear_to_srgb(const float3 color)
{
   const float3 srgb_low = color * 12.92;
   const float3 srgb_hi = (pow(abs(color), 1.0 / 2.4) * 1.055) - 0.055;

   return (color <= 0.0031308) ? srgb_low : srgb_hi;
}

#endif