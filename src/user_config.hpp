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

struct Effects_user_config {
   bool bloom = true;
   bool vignette = true;
   bool film_grain = true;
   bool colored_film_grain = true;
   bool ssao = true;
   SSAO_quality ssao_quality = SSAO_quality::medium;
};

struct User_config {
   User_config() = default;

   explicit User_config(const std::string& path) noexcept;

   bool enabled = true;

   struct {
      std::uint32_t screen_percent = 100;
      bool allow_tearing = true;
      bool centred = true;
      bool dpi_aware = true;
      bool dpi_scaling = true;
      bool treat_800x600_as_interface = true;
      bool windowed_interface = false;
      bool scalable_fonts = true;
      bool enable_game_perceived_resolution_override = false;
      std::uint32_t game_perceived_resolution_override_width = 1920;
      std::uint32_t game_perceived_resolution_override_height = 1080;
   } display;

   struct {
      GPU_selection_method gpu_selection_method =
         GPU_selection_method::highest_performance;
      Antialiasing_method antialiasing_method = Antialiasing_method::cmaa2;
      Anisotropic_filtering anisotropic_filtering = Anisotropic_filtering::x16;
      Refraction_quality refraction_quality = Refraction_quality::medium;
      bool enable_oit = false;
      bool enable_alternative_postprocessing = true;
      bool enable_scene_blur = true;
      bool enable_16bit_color_rendering = true;
      bool disable_light_brightness_rescaling = false;
      bool enable_user_effects_config = false;
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

      std::filesystem::path scalable_font_name = L"ariblk.ttf";
   } developer;

   void show_imgui() noexcept;

private:
   void parse_file(const std::string& path);
};

extern User_config user_config;

inline auto gpu_selection_method_from_string(const std::string_view string) noexcept
   -> GPU_selection_method
{
   if (string == "Highest Performance") {
      return GPU_selection_method::highest_performance;
   }
   else if (string == "Lowest Power Usage") {
      return GPU_selection_method::lowest_power_usage;
   }
   else if (string == "Highest Feature Level") {
      return GPU_selection_method::highest_feature_level;
   }
   else if (string == "Most Memory") {
      return GPU_selection_method::most_memory;
   }
   else if (string == "Use CPU") {
      return GPU_selection_method::use_cpu;
   }

   return GPU_selection_method::highest_performance;
}

inline auto to_string(const Antialiasing_method quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
      return "none"s;
   case Antialiasing_method::cmaa2:
      return "CMAA2"s;
   case Antialiasing_method::msaax4:
      return "MSAAx4"s;
   case Antialiasing_method::msaax8:
      return "MSAAx8"s;
   }

   std::terminate();
}

inline auto to_sample_count(const Antialiasing_method quality) noexcept -> std::size_t
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
   case Antialiasing_method::cmaa2:
      return 1;
   case Antialiasing_method::msaax4:
      return 4;
   case Antialiasing_method::msaax8:
      return 8;
   }

   std::terminate();
}

inline auto aa_method_from_string(const std::string_view string) noexcept -> Antialiasing_method
{
   if (string == to_string(Antialiasing_method::none)) {
      return Antialiasing_method::none;
   }
   else if (string == to_string(Antialiasing_method::cmaa2)) {
      return Antialiasing_method::cmaa2;
   }
   else if (string == to_string(Antialiasing_method::msaax4)) {
      return Antialiasing_method::msaax4;
   }
   else if (string == to_string(Antialiasing_method::msaax8)) {
      return Antialiasing_method::msaax8;
   }
   else {
      return Antialiasing_method::none;
   }
}

inline auto to_string(const Anisotropic_filtering filtering) noexcept -> std::string
{
   using namespace std::literals;

   switch (filtering) {
   case Anisotropic_filtering::off:
      return "off"s;
   case Anisotropic_filtering::x2:
      return "x2"s;
   case Anisotropic_filtering::x4:
      return "x4"s;
   case Anisotropic_filtering::x8:
      return "x8"s;
   case Anisotropic_filtering::x16:
      return "x16"s;
   }

   std::terminate();
}

inline auto to_sample_count(const Anisotropic_filtering filtering) noexcept -> std::size_t
{
   using namespace std::literals;

   switch (filtering) {
   case Anisotropic_filtering::x2:
      return 2;
   case Anisotropic_filtering::x4:
      return 4;
   case Anisotropic_filtering::x8:
      return 8;
   case Anisotropic_filtering::x16:
      return 16;
   default:
      return 1;
   }
}

inline auto anisotropic_filtering_from_string(const std::string_view string) noexcept
   -> Anisotropic_filtering
{
   if (string == to_string(Anisotropic_filtering::off)) {
      return Anisotropic_filtering::off;
   }
   else if (string == to_string(Anisotropic_filtering::x2)) {
      return Anisotropic_filtering::x2;
   }
   else if (string == to_string(Anisotropic_filtering::x4)) {
      return Anisotropic_filtering::x4;
   }
   else if (string == to_string(Anisotropic_filtering::x8)) {
      return Anisotropic_filtering::x8;
   }
   else if (string == to_string(Anisotropic_filtering::x16)) {
      return Anisotropic_filtering::x16;
   }
   else {
      return Anisotropic_filtering::off;
   }
}

inline auto to_string(const SSAO_quality quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case SSAO_quality::fastest:
      return "Fastest"s;
   case SSAO_quality::fast:
      return "Fast"s;
   case SSAO_quality::medium:
      return "Medium"s;
   case SSAO_quality::high:
      return "High"s;
   case SSAO_quality::highest:
      return "Highest"s;
   }

   std::terminate();
}

inline auto ssao_quality_from_string(const std::string_view string) noexcept -> SSAO_quality
{
   if (string == to_string(SSAO_quality::fastest)) {
      return SSAO_quality::fastest;
   }
   else if (string == to_string(SSAO_quality::fast)) {
      return SSAO_quality::fast;
   }
   else if (string == to_string(SSAO_quality::medium)) {
      return SSAO_quality::medium;
   }
   else if (string == to_string(SSAO_quality::high)) {
      return SSAO_quality::high;
   }
   else if (string == to_string(SSAO_quality::highest)) {
      return SSAO_quality::highest;
   }
   else {
      return SSAO_quality::medium;
   }
}

inline auto to_string(const Refraction_quality quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case Refraction_quality::low:
      return "Low"s;
   case Refraction_quality::medium:
      return "Medium"s;
   case Refraction_quality::high:
      return "High"s;
   case Refraction_quality::ultra:
      return "Ultra"s;
   }

   std::terminate();
}

inline auto refraction_quality_from_string(const std::string_view string) noexcept
   -> Refraction_quality
{
   if (string == to_string(Refraction_quality::low)) {
      return Refraction_quality::low;
   }
   else if (string == to_string(Refraction_quality::medium)) {
      return Refraction_quality::medium;
   }
   else if (string == to_string(Refraction_quality::high)) {
      return Refraction_quality::high;
   }
   else if (string == to_string(Refraction_quality::ultra)) {
      return Refraction_quality::ultra;
   }
   else {
      return Refraction_quality::medium;
   }
}

constexpr auto to_scale_factor(const Refraction_quality quality) noexcept -> int
{
   switch (quality) {
   case Refraction_quality::low:
      return 4;
   case Refraction_quality::medium:
      return 2;
   case Refraction_quality::high:
      return 2;
   case Refraction_quality::ultra:
      return 1;
   }

   return 2;
}

constexpr bool use_depth_refraction_mask(const Refraction_quality quality) noexcept
{
   switch (quality) {
   case Refraction_quality::low:
      return false;
   case Refraction_quality::medium:
      return false;
   case Refraction_quality::high:
      return true;
   case Refraction_quality::ultra:
      return true;
   }

   return false;
}

}
