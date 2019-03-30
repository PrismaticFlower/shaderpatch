#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

namespace sp::core::cb {

struct Scene_tag {
};
struct Draw_tag {
};
struct Fixedfunction_tag {
};
struct Skin_tag {
};
struct Draw_ps_tag {
};

static constexpr Scene_tag scene{};
static constexpr Draw_tag draw{};
static constexpr Fixedfunction_tag fixedfunction{};
static constexpr Skin_tag skin{};
static constexpr Draw_ps_tag draw_ps{};

struct alignas(16) Scene {
   std::array<glm::vec4, 4> projection_matrix;
   glm::vec3 vs_view_positionWS;
   float _buffer_padding0;
   glm::vec4 fog_info;
   glm::vec2 near_scene_fade;
   float vs_lighting_scale;
   float _buffer_padding2;
   std::array<glm::vec4, 3> shadow_map_transform;
   std::uint32_t vertex_color_srgb;
   float time;
   float tessellation_resolution_factor;
   float _buffer_padding1;
};

static_assert(sizeof(Scene) == 176);

struct alignas(16) Draw {
   glm::vec4 normaltex_decompress;
   glm::vec4 position_decompress_min;
   glm::vec4 position_decompress_max;
   glm::vec4 color_state;
   std::array<glm::vec4, 3> world_matrix;
   glm::vec4 light_ambient_color_top;
   glm::vec4 light_ambient_color_bottom;
   glm::vec4 light_directional_0_color;
   glm::vec4 light_directional_0_dir;
   glm::vec4 light_directional_1_color;
   glm::vec4 light_directional_1_dir;
   glm::vec4 light_point_0_color;
   glm::vec4 light_point_0_pos;
   glm::vec4 light_point_1_color;
   glm::vec4 light_point_1_pos;
   glm::vec4 overlapping_lights[4];
   glm::vec4 light_proj_color;
   glm::vec4 light_proj_selector;
   std::array<glm::vec4, 4> light_proj_matrix;
   glm::vec4 material_diffuse_color;
   glm::vec4 custom_constants[9];
};

static_assert(sizeof(Draw) == 592);

struct alignas(16) Fixedfunction {
   glm::vec4 texture_factor;
   glm::vec2 inv_resolution;
   std::array<float, 2> _buffer_padding;
};

static_assert(sizeof(Fixedfunction) == 32);

struct alignas(16) Skin {
   std::array<std::array<glm::vec4, 3>, 15> bone_matrices;
};

static_assert(sizeof(Skin) == 720);

struct alignas(16) Draw_ps {
   std::array<glm::vec4, 5> ps_custom_constants;
   glm::vec3 ps_view_positionWS;
   float ps_lighting_scale;
   glm::vec4 rt_resolution; // x = width, y = height, z = 1 / width, w = 1 / height
   glm::vec3 fog_color;
   float stock_tonemap_state;
   float rt_multiply_blending_state;
   std::int32_t cube_projtex;
   std::int32_t fog_enabled;
   std::int32_t _buffer_padding2;
};

static_assert(sizeof(Draw_ps) == 144);

}
