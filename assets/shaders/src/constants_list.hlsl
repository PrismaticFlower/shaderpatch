#ifndef CONSTANTS_LIST_INCLUDED
#define CONSTANTS_LIST_INCLUDED

// This file is a listing of SWBFII's shader constants/uniforms as in 
// pcredvertexshaderconstants.h. Along with the comments on the contents
// of the constants.

// world space to projection space matrix
float4x4 projection_matrix : register(vs, c[2]);

// the position of the camera
float4 world_view_position : register(c[6]);

// (camera fog scale, camera fog offset, world fog scale, world fog offset)
float4 fog_info : register(c[7]);

// (nearfade scale, nearfade offset, lighting scale, 1.0)
float4 near_scene_fade : register(c[8]);

// uses the same register as near_scene_fade, so may be no different or
// it might be. The original authors of the shaders made the distinction
// so this does as well.
static const float4 hdr_info = near_scene_fade;

// shadow map transform
float4 shadow_map_transform[3] : register(c[9]);

// (normal decompress = 2 or 1, -1 or 0, texture decompress = 1 / 0x0800 or 1, 1)
float4 normaltex_decompress : register(c[12]);

// min_pos = ((bbox (max - min) * 0.5 / 0x7FFF) or (1, 1, 1), 0)
float4 position_decompress_min : register(c[13]);
// max_pos = ((bbox (max + min) * 0.5 / 0x7FFF) or (0, 0, 0), 1)
float4 position_decompress_max : register(c[14]);

// Pandemic: whether vertex colors are lighting or material colors
// (1, 0, 0, 0)
float4 color_state : register(c[15]);

// object space to world space matrix
float4x3 world_matrix : register(c[16]);

// Pandemic: ambient color interpolated with ambient color1 using world 
// normal y component - applied in the transform fragment (i.e. lighting 
// fragment not needed)
float4 light_ambient_color_top : register(c[19]);

// Pandemic: ambient color interpolated with ambient color0 using world 
// normal y component - applied in the transform fragment (i.e. lighting 
// fragment not needed)
float4 light_ambient_color_bottom : register(c[20]);

// directional light 0 color
float4 light_directional_0_color : register(c[21]);

// directional light 0 normalized world space direction
float4 light_directional_0_dir : register(c[22]);

// directional light 1 color
float4 light_directional_1_color : register(c[23]);

// directional light 1 normalized world space direction
float4 light_directional_1_dir : register(c[24]);

// point light 0 color, intensity in alpha value
float4 light_point_0_color : register(c[25]);

// point light 0 world space position
float4 light_point_0_pos : register(c[26]);

// point light 1 color, intensity in alpha value
float4 light_point_1_color : register(c[27]);

// point light 1 world space position
float4 light_point_1_pos : register(c[28]);

float4 overlapping_lights[4] : register(c[29]);

// point light 2 color, intensity in alpha value 
// (shares register with light_spot_color)
static const float4 light_point_2_color = overlapping_lights[0];

// point light 2 world space position
// (shares register with light_spot_pos)
static const float4 light_point_2_pos = overlapping_lights[1];

// point light 3 color, intensity in alpha value
// (shares register with light_spot_dir)
static const float4 light_point_3_color = overlapping_lights[2];

// point light 3 world space position
// (shares register with light_spot_params)
static const float4 light_point_3_pos = overlapping_lights[3];

// spot light color, intensity in alpha value
static const float4 light_spot_color = overlapping_lights[0];

// spot light position, w = 1 / r^2
static const float4 light_spot_pos = overlapping_lights[1];

// spot light direction
static const float4 light_spot_dir = overlapping_lights[2];

// spot light params
// (x = half outer cone angle, y = half inner cone angle, 
//  z = 1 / (cos(y) - cos(x)), w = falloff)
static const float4 light_spot_params = overlapping_lights[3];

// projected light color
float4 light_proj_color : register(c[33]);

// Pandemic: selects which light use for the projection texture
float4 light_proj_selector : register(c[34]);

// projected light matrix
float4x4 light_proj_matrix : register(c[35]);

// Pandemic: material diffuse color (x tweak color)
float4 material_diffuse_color : register(c[39]);

// registers 40 to 50 are set aside as "custom constants"
#define CUSTOM_CONST_MIN 40
#define CUSTOM_CONST_MAX 50

// bone matrices
float4 bone_matrices[45] : register(vs, c[51]);


#endif