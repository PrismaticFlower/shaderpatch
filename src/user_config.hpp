#pragma once

#include "imgui/imgui_ext.hpp"
#include "logger.hpp"

#include <cstdint>
#include <iomanip>
#include <string>
#include <string_view>

#include <imgui.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

enum class Antialiasing_method { none, msaax2, msaax4, msaax8 };

enum class Anisotropic_filtering { off, x2, x4, x8, x16 };

struct Effects_user_config {
   bool enabled = true;
   bool bloom = true;
   bool vignette = true;
   bool film_grain = true;
   bool colored_film_grain = true;
};

struct User_config {
   User_config() = default;

   explicit User_config(const std::string& path) noexcept;

   bool enabled = true;

   struct {
      std::uint32_t screen_percent = 100;
      bool allow_tearing = true;
      bool centred = false;
   } display;

   struct {
      Antialiasing_method antialiasing_method = Antialiasing_method::msaax4;
      Anisotropic_filtering anisotropic_filtering = Anisotropic_filtering::x16;
      bool enable_tessellation = true;
      bool enable_user_effects_config = false;
      std::string user_effects_config;
   } graphics;

   Effects_user_config effects{};

   struct {
      std::uintptr_t toggle_key{0};

      bool allow_event_queries = false;
      bool use_d3d11_debug_layer = false;
   } developer;

   void show_imgui() noexcept;

private:
   void parse_file(const std::string& path);
};

extern User_config user_config;

inline auto to_string(const Antialiasing_method quality) noexcept -> std::string
{
   using namespace std::literals;

   switch (quality) {
   case Antialiasing_method::none:
      return "none"s;
   case Antialiasing_method::msaax2:
      return "MSAAx2"s;
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
      return 1;
   case Antialiasing_method::msaax2:
      return 2;
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
   else if (string == to_string(Antialiasing_method::msaax2)) {
      return Antialiasing_method::msaax2;
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

}
