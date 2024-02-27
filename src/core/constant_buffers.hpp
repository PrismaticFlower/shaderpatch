#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

namespace sp::core::cb {

// Evil macro. Takes the type of the constant buffer and the first non-game
// constant in the type and returns the number of game constants in the buffer.
#define CB_MAX_GAME_CONSTANTS(Type, first_patch_constant)                      \
   (sizeof(Type) - (sizeof(Type) - offsetof(Type, first_patch_constant))) /    \
      sizeof(glm::vec4);

struct Scene_tag {};
struct Draw_tag {};
struct Fixedfunction_tag {};
struct Skin_tag {};
struct Draw_ps_tag {};

static constexpr Scene_tag scene{};
static constexpr Draw_tag draw{};
static constexpr Fixedfunction_tag fixedfunction{};
static constexpr Skin_tag skin{};
static constexpr Draw_ps_tag draw_ps{};

struct alignas(16) Scene {
   std::array<glm::vec4, 4> projection_matrix;
   glm::vec3 vs_view_positionWS;
   float _padding0;
   glm::vec4 fog_info;
   float near_scene_fade_scale;
   float near_scene_fade_offset;
   float vs_lighting_scale;
   std::uint32_t _padding1{};
   std::array<glm::vec4, 3> shadow_map_transform;
   glm::vec2 pixel_offset;
   std::uint32_t input_color_srgb;
   std::uint32_t vs_use_soft_skinning;
   float time;
   std::uint32_t particle_texture_scale;
   float prev_near_scene_fade_scale;
   float prev_near_scene_fade_offset;
};

constexpr auto scene_game_count = CB_MAX_GAME_CONSTANTS(Scene, pixel_offset);

static_assert(sizeof(Scene) == 192);
static_assert(scene_game_count == 10);

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

constexpr auto draw_game_count = sizeof(Draw) / 16;

static_assert(sizeof(Draw) == 592);
static_assert(draw_game_count == 37);

struct alignas(16) Fixedfunction {
   glm::vec4 texture_factor;
   glm::vec2 inv_resolution;
   std::array<float, 2> _buffer_padding;
};

static_assert(sizeof(Fixedfunction) == 32);

struct alignas(16) Skin {
   std::array<std::array<glm::vec4, 3>, 15> bone_matrices;
};

constexpr auto skin_game_count = sizeof(Skin) / 16;

static_assert(sizeof(Skin) == 720);
static_assert(skin_game_count == 45);

struct alignas(16) Draw_ps {
   std::array<glm::vec4, 5> ps_custom_constants;
   glm::vec3 ps_view_positionWS;
   float ps_lighting_scale;
   glm::vec4 rt_resolution; // x = width, y = height, z = 1 / width, w = 1 / height
   glm::vec3 fog_color;
   std::uint32_t light_active = 0;
   std::uint32_t light_active_point_count = 0;
   std::uint32_t light_active_spot = 0;
   std::uint32_t additive_blending;
   std::uint32_t cube_projtex;
   std::uint32_t fog_enabled;
   std::uint32_t limit_normal_shader_bright_lights;
   std::uint32_t input_color_srgb;
   std::uint32_t supersample_alpha_test;
   float time_seconds;
   std::array<uint32_t, 2> padding;
};

constexpr auto draw_ps_game_count = CB_MAX_GAME_CONSTANTS(Draw_ps, ps_view_positionWS);

static_assert(sizeof(Draw_ps) == 176);
static_assert(draw_ps_game_count == 5);

#undef CB_MAX_GAME_CONSTANTS

struct alignas(16) Team_colors {
   alignas(16) glm::vec3 friend_color;
   alignas(16) glm::vec3 friend_health_color;
   alignas(16) glm::vec3 friend_corsshair_dot_color;
   alignas(16) glm::vec3 foe_color;
   alignas(16) glm::vec3 foe_text_color;
   alignas(16) glm::vec3 foe_health_color;
   alignas(16) glm::vec3 foe_crosshair_dot_color;
};

static_assert(sizeof(Team_colors) == 112);

}
