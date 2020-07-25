#ifndef CONSTANTS_LIST_INCLUDED
#define CONSTANTS_LIST_INCLUDED

cbuffer SceneConstants : register(b0)
{
   float4x4 projection_matrix;
   float3 vs_view_positionWS;
   float _bufferPadding0;
   float depth_fog_scale;
   float depth_fog_offset;
   float height_fog_scale;
   float height_fog_offset;
   float near_fade_scale;
   float near_fade_offset;
   float vs_lighting_scale;
   float _bufferPadding1;
   float4x3 shadow_map_transform;
   float2 vs_pixel_offset;
   bool vertex_color_srgb;
   float time;
   float tessellation_resolution_factor;
   bool particle_texture_scale;
   float prev_near_fade_scale;
   float prev_near_fade_offset;
}

cbuffer DrawConstants : register(b1)
{
   float4 normaltex_decompress;
   float4 position_decompress_min;
   float4 position_decompress_max;
   float4 color_state; // whether vertex colors are lighting or material colors (0, 1, 1, 0) or (0, 1, 1, 0)
   float4x3 world_matrix;
   float4 light_ambient_color_top;
   float4 light_ambient_color_bottom;
   float4 light_packed_constants[12];
   float4 light_proj_color;
   float4 light_proj_selector;
   float4x4 light_proj_matrix;
   float4 material_diffuse_color;
   float4 custom_constants[9];
}

cbuffer FixedfuncConstants : register(b2)
{
   float4 ff_texture_factor;
   float2 ff_inv_resolution;
}

tbuffer SkinConstants : register(t0)
{
   float4x3 bone_matrices[15];
}

cbuffer PSDrawConstants : register(b0)
{
   float4 ps_custom_constants[5];
   float3 ps_view_positionWS;
   float ps_lighting_scale;
   float4 rt_resolution; // x = width, y = height, z = 1 / width, w = 1 / height
   float3 fog_color;
   float rcp_sample_count;
   bool light_active;
   uint light_active_point_count;
   bool light_active_spot;
   bool additive_blending;
   bool cube_projtex;
   bool fog_enabled;
   bool limit_normal_shader_bright_lights;
   float time_seconds;
}

#ifdef __PIXEL_SHADER__
static const float3 view_positionWS = ps_view_positionWS;
static const float lighting_scale = ps_lighting_scale;
#else
static const float3 view_positionWS = vs_view_positionWS;
static const float lighting_scale = vs_lighting_scale;
#endif

const static uint light_directional_color_offset = 0;
const static uint light_directional_dir_offset = 1;
const static uint light_point_color_offset = 4;
const static uint light_point_pos_offset = 5;
const static uint light_spot_offset = 8;

float4 light_directional_color(uint index)
{
   return light_packed_constants[light_directional_color_offset + (index * 2)];
}

float3 light_directional_dir(uint index)
{
   return light_packed_constants[light_directional_dir_offset + (index * 2)].xyz;
}

float4 light_point_color(uint index)
{
   return light_packed_constants[light_point_color_offset + (index * 2)];
}

float3 light_point_pos(uint index)
{
   return light_packed_constants[light_point_pos_offset + (index * 2)].xyz;
}

float light_point_inv_range_sqr(uint index)
{
   return light_packed_constants[light_point_pos_offset + (index * 2)].w;
}

static const float4 light_spot_color = light_packed_constants[light_spot_offset];
static const float3 light_spot_pos =
   light_packed_constants[light_spot_offset + 1].xyz;
static const float light_spot_inv_range_sqr =
   light_packed_constants[light_spot_offset + 1].w;
static const float4 light_spot_dir = light_packed_constants[light_spot_offset + 2];

// spot light params
// (x = half outer cone angle, y = half inner cone angle,
//  z = 1 / (cos(y) - cos(x)), w = falloff)
static const float4 light_spot_params = light_packed_constants[light_spot_offset + 3];

#ifdef __VERTEX_SHADER__
#define MATERIAL_CB_INDEX b3
#elif defined(__PIXEL_SHADER__)
#define MATERIAL_CB_INDEX b2
#else
#define MATERIAL_CB_INDEX b1
#endif

#endif