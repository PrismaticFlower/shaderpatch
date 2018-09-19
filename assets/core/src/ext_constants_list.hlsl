#ifndef EXT_CONSTANTS_LIST_INCLUDED
#define EXT_CONSTANTS_LIST_INCLUDED

// x fog start, y = fog end, z = 1 / (fog end - fog start)
float3 fog_range : register(ps, c[51]);
float3 fog_color : register(ps, c[52]);

// x = width, y = height, z = 1 / width, w = 1 / height
float4 rt_resolution : register(ps, c[53]);;

float time : register(vs, c[97])
           : register(ps, c[54]);

float4 material_constants[8] : register(c[128]);

// Runtime State Constants
bool directional_lights : register(b0);
bool point_light_0 : register(b1);
bool point_light_1 : register(b2);
bool point_light_23 : register(b3);
bool spot_light : register(b4);
bool fog_enabled : register(b5);
bool cube_map_light_projection : register(b6);

float2 linear_state : register(c[98]);

const static float color_gamma = linear_state.x;
const static float tonemap_state = linear_state.y;

float rt_multiply_blending : register(ps, c[99]);

uniform TextureCube<float3> cube_projected_texture : register(ps_3_0, s15);
uniform SamplerState projected_texture_sampler;

// Compile Time Lighting Constants 

#ifdef __PS_FORCE_RUNTIME_LIGHTS_CONSTANTS__

const static bool ps_light_active_directional = directional_lights;
const static bool ps_light_active_point_0 = point_light_0;
const static bool ps_light_active_point_1 = point_light_1;
const static bool ps_light_active_point_23 = point_light_23;
const static bool ps_light_active_spot_light = spot_light;

#else

#ifdef __PS_LIGHT_ACTIVE_DIRECTIONAL__
const static bool ps_light_active_directional = true;
#else
const static bool ps_light_active_directional = false;
#endif
#ifdef __PS_LIGHT_ACTIVE_POINT_0__
const static bool ps_light_active_point_0 = true;
#else
const static bool ps_light_active_point_0 = false;
#endif
#ifdef __PS_LIGHT_ACTIVE_POINT_1__
const static bool ps_light_active_point_1 = true;
#else
const static bool ps_light_active_point_1 = false;
#endif
#ifdef __PS_LIGHT_ACTIVE_POINT_23__
const static bool ps_light_active_point_23 = true;
#else
const static bool ps_light_active_point_23 = false;
#endif
#ifdef __PS_LIGHT_ACTIVE_SPOT__
const static bool ps_light_active_spot_light = true;
#else
const static bool ps_light_active_spot_light = false;
#endif

#endif

#endif