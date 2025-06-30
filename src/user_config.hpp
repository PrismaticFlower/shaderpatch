#pragma once

#include "imgui/imgui_ext.hpp"
#include "logger.hpp"

#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <string>
#include <string_view>

#include "imgui/imgui.h"

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

enum class GPU_selection_method : std::int8_t {
   highest_performance,
   lowest_power_usage,
   highest_feature_level,
   most_memory,
   use_cpu
};

enum class Antialiasing_method : std::int8_t { none, cmaa2, msaax4, msaax8 };

enum class Anisotropic_filtering : std::int8_t { off, x2, x4, x8, x16 };

enum class SSAO_quality : std::int8_t { fastest, fast, medium, high, highest };

enum class Refraction_quality : std::int8_t { low, medium, high, ultra };

enum class Aspect_ratio_hud : std::int8_t {
   stretch_4_3,
   centre_4_3,
   stretch_16_9,
   centre_16_9
};

enum class DOF_quality {
   ultra_performance,
   performance,
   quality,
   ultra_quality
};

struct Effects_user_config {
   bool bloom = true;
   bool vignette = true;
   bool film_grain = true;
   bool colored_film_grain = true;
   bool ssao = true;
   SSAO_quality ssao_quality = SSAO_quality::medium;
   bool dof = true;
   DOF_quality dof_quality = DOF_quality::quality;
};

struct User_config {
   User_config() = default;

   explicit User_config(const std::string& path) noexcept;

   bool enabled = true;

   struct {
      bool allow_tearing = false;
      bool dpi_aware = true;
      bool dpi_scaling = true;
      bool dsr_vsr_scaling = true;
      bool treat_800x600_as_interface = true;
      bool stretch_interface = false;
      bool scalable_fonts = true;
      bool aspect_ratio_hack = false;
      Aspect_ratio_hud aspect_ratio_hack_hud = Aspect_ratio_hud::centre_4_3;
      bool enable_game_perceived_resolution_override = false;
      std::uint32_t game_perceived_resolution_override_width = 1920;
      std::uint32_t game_perceived_resolution_override_height = 1080;
      bool override_resolution = false;
      std::uint32_t override_resolution_screen_percent = 100;
   } display;

   struct {
      std::uint32_t extra_ui_scaling = 100;
      std::array<std::uint8_t, 3> friend_color = {1, 86, 213};
      std::array<std::uint8_t, 3> foe_color = {223, 32, 32};
   } ui;

   struct {
      GPU_selection_method gpu_selection_method =
         GPU_selection_method::highest_performance;
      Antialiasing_method antialiasing_method = Antialiasing_method::cmaa2;
      Anisotropic_filtering anisotropic_filtering = Anisotropic_filtering::x16;
      Refraction_quality refraction_quality = Refraction_quality::medium;
      bool enable_oit = false;
      bool enable_alternative_postprocessing = true;
      bool enable_scene_blur = true;
      bool enable_16bit_color_rendering = false;
      bool disable_light_brightness_rescaling = false;
      bool enable_user_effects_config = false;
      bool supersample_alpha_test = false;
      bool allow_vertex_soft_skinning = false;
      bool use_d3d11on12 = false;
      std::string user_effects_config;
   } graphics;

   Effects_user_config effects{};

   struct {
      std::uintptr_t toggle_key{0};

      bool monitor_bfront2_log = false;
      bool allow_event_queries = false;
      bool use_d3d11_debug_layer = false;
      bool use_dxgi_1_2_factory = false;

      std::filesystem::path shader_cache_path =
         LR"(.\data\shaderpatch\.shader_dxbc_cache)";
      std::filesystem::path shader_definitions_path =
         LR"(.\data\shaderpatch\shaders\definitions)";
      std::filesystem::path shader_source_path = LR"(.\data\shaderpatch\shaders\src)";

      std::filesystem::path material_scripts_path =
         LR"(.\data\shaderpatch\scripts\material)";

      std::filesystem::path scalable_font_name = L"ariblk.ttf";
   } developer;

   void show_imgui() noexcept;

private:
   void parse_file(const std::string& path);
};

extern User_config user_config;

auto gpu_selection_method_from_string(const std::string_view string) noexcept
   -> GPU_selection_method;

auto to_string(const Antialiasing_method quality) noexcept -> std::string;

auto to_sample_count(const Antialiasing_method quality) noexcept -> std::size_t;

auto aa_method_from_string(const std::string_view string) noexcept -> Antialiasing_method;

auto to_string(const Anisotropic_filtering filtering) noexcept -> std::string;

auto to_sample_count(const Anisotropic_filtering filtering) noexcept -> std::size_t;

auto anisotropic_filtering_from_string(const std::string_view string) noexcept
   -> Anisotropic_filtering;

auto to_string(const SSAO_quality quality) noexcept -> std::string;

auto ssao_quality_from_string(const std::string_view string) noexcept -> SSAO_quality;

auto to_string(const DOF_quality quality) noexcept -> std::string;

auto dof_quality_from_string(const std::string_view string) noexcept -> DOF_quality;

auto to_string(const Refraction_quality quality) noexcept -> std::string;

auto refraction_quality_from_string(const std::string_view string) noexcept
   -> Refraction_quality;

auto to_scale_factor(const Refraction_quality quality) noexcept -> int;

bool use_depth_refraction_mask(const Refraction_quality quality) noexcept;

auto to_string(const Aspect_ratio_hud hud) noexcept -> std::string;

auto aspect_ratio_hud_from_string(const std::string_view string) noexcept
   -> Aspect_ratio_hud;

}
