#ifndef EXT_CONSTANTS_LIST_INCLUDED
#define EXT_CONSTANTS_LIST_INCLUDED


// x fog start, y = fog end, z = 1 / (fog end - fog start)
float3 fog_range : register(ps, c[51]);
float3 fog_color : register(ps, c[52]);

// x = width, y = height, z = 1 / width, w = 1 / height
float4 rt_resolution : register(ps, c[53]);

// world space to projection space matrix
float4x4 ps_projection_matrix : register(ps, c[54]);


float time : register(vs, c[97]);

bool directional_lights : register(b0);
bool point_light_0 : register(b1);
bool point_light_1 : register(b2);
bool point_light_23 : register(b3);
bool spot_light : register(b4);
bool fog_enabled : register(b5);
bool cube_map_light_projection : register(b6);

uniform samplerCUBE cube_light_texture : register(s15);

#endif