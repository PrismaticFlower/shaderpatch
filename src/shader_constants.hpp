#pragma once

namespace sp::constants {

// extension ps constants
namespace ps {
constexpr auto light_state = 0u;
constexpr auto fog_enabled = 5u;
constexpr auto cubemap_projection = 6u;

constexpr auto fog_range = 51u;
constexpr auto fog_color = 52u;

constexpr auto rt_resolution = 53u;

constexpr auto projection_matrix = 54u;

constexpr auto material_constants_start = 128u;
constexpr auto material_constants_end = 136u;
}

// extension vs constants
namespace vs {
constexpr auto time = 97u;

constexpr auto material_constants_start = 128u;
constexpr auto material_constants_end = 136u;
}

// stock vertex shader constants
namespace stock {
// constant that is actually constant with the fixed values: (0.0, 0.5, 1.0, -1.0)
constexpr auto const0 = 0u;
// constant that is actually constant with the fixed values: (2.0, 0.25,0.5, index_decompress = 765.001)
constexpr auto const1 = 1u;

// projection matrix
constexpr auto projection_matrix = 2u;

constexpr auto world_viewpos = 6u;

// (camfog scale, camfog offset, worldfog scale, worldfog offset)
constexpr auto fog = 7u;

// (nearfade scale, nearfade offset, lighting scale, 1.0)
constexpr auto nearscenefade = 8u;
constexpr auto hdr = nearscenefade;

// shadow map transform
constexpr auto shadowmap_transform_u = 9u;
constexpr auto shadowmap_transform_v = 10u;
constexpr auto shadowmap_transform_w = 11u;

// decompression constants
constexpr auto normaltex_decompress = 12u;
constexpr auto pos_decompress0 = 13u;
constexpr auto pos_decompress1 = 14u;

// whether vertex colors are lighting or material colors (1,?,?,?)
constexpr auto color_state = 15u;

constexpr auto worldview_matrix = 16u;

constexpr auto ambient_color_top = 19u;
constexpr auto ambient_color_bottom = 20u;

// directional 0 light color
constexpr auto light_directional0_color = 21u;
constexpr auto light_directional0_dir = 22u;

constexpr auto light_directional1_color = 23u;
constexpr auto light_directional1_dir = 24u;

// point light 0 color, intensity
constexpr auto light_point0_color = 25u;
// point light 0 world space position
constexpr auto light_point0_pos = 26u;

constexpr auto light_point1_color = 27u;
constexpr auto light_point1_pos = 28u;
constexpr auto light_point2_color = 29u;
constexpr auto light_point2_pos = 30u;
constexpr auto light_point3_color = 31u;
constexpr auto light_point3_pos = 32u;

// spot light color, w = intensity
constexpr auto light_spot_color = light_point2_color;
// spot light pos, w = 1 / r^2
constexpr auto light_spot_pos = light_point2_pos;
// spot light direction
constexpr auto light_spot_dir = light_point3_color;
// spot light params [x = half outer cone angle, y = half inner cone angle, z = 1/(cos(y) - cos(x)), w = falloff]
constexpr auto light_spot_params = light_point3_pos;

constexpr auto light_proj_color = 33u;
constexpr auto light_proj_selector = 34u;
constexpr auto light_proj_matrix = 35u;

constexpr auto material_diffuse_color = 39u;

// custom shader constants range whose meaning depends on the active shader
constexpr auto custom_start = 40u;
constexpr auto custom_end = 50u;

// array of 3x4 skinning matrices
constexpr auto bones_start = 51u;
constexpr auto bones_end = 96u;
}
}
