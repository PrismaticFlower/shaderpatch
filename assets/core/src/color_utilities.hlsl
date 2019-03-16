#ifndef COLOR_UTILS_INCLUDED
#define COLOR_UTILS_INCLUDED

float3 srgb_to_linear(const float3 color)
{
   return (color < 0.04045) ? color / 12.92 : pow(abs((color + 0.055)) / 1.055, 2.4);
}

float3 linear_to_srgb(const float3 color)
{
   return (color < 0.0031308) ? color * 12.92 : 1.055 * pow(abs(color), 1.0 / 2.4) - 0.055;
}

#endif