#pragma once

#include "imgui/imgui_ext.hpp"
#include "logger.hpp"

#include <cstdint>
#include <iomanip>
#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <imgui.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

enum class Antialiasing_method { none, msaax2, msaax4, msaax8 };

enum class Color_quality { normal, high, ultra };

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

inline auto aa_method_from_string(std::string_view string) noexcept -> Antialiasing_method
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

inline auto to_string(const Color_quality detail) noexcept -> std::string
{
   using namespace std::literals;

   switch (detail) {
   case Color_quality::normal:
      return "normal"s;
   case Color_quality::high:
      return "high"s;
   case Color_quality::ultra:
      return "ultra"s;
   }

   std::terminate();
}

inline auto color_quality_from_string(std::string_view string) noexcept -> Color_quality
{
   if (string == to_string(Color_quality::normal)) {
      return Color_quality::normal;
   }
   else if (string == to_string(Color_quality::high)) {
      return Color_quality::high;
   }
   else if (string == to_string(Color_quality::ultra)) {
      return Color_quality::ultra;
   }
   else {
      return Color_quality::normal;
   }
}

struct Effects_user_config {
   bool enabled = true;
   bool bloom = true;
   bool vignette = true;
   bool film_grain = true;
   bool colored_film_grain = true;

   Color_quality color_quality = Color_quality::high;
};

struct User_config {
   User_config() = default;

   explicit User_config(const std::string& path) noexcept;

   struct {
      std::uint32_t screen_percent = 100;
      bool allow_tearing = true;
      bool centred = false;
   } display;

   struct {
      Antialiasing_method antialiasing_method = Antialiasing_method::msaax4;
      std::size_t antialiasing_sample_count = to_sample_count(antialiasing_method);
   } graphics;

   Effects_user_config effects{};

   struct {
      std::uintptr_t toggle_key{0};

      bool unlock_fps = false;
      bool allow_event_queries = false;
      bool force_d3d11_debug_layer = false;
   } developer;

   void show_imgui() noexcept
   {
      ImGui::Begin("User Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

      if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
         graphics.antialiasing_method = aa_method_from_string(
            ImGui::StringPicker("Anti-Aliasing Method",
                                std::string{to_string(graphics.antialiasing_method)},
                                std::initializer_list<std::string>{
                                   to_string(Antialiasing_method::none),
                                   to_string(Antialiasing_method::msaax2),
                                   to_string(Antialiasing_method::msaax4),
                                   to_string(Antialiasing_method::msaax8)}));

         graphics.antialiasing_sample_count =
            to_sample_count(graphics.antialiasing_method);
      }

      if (ImGui::CollapsingHeader("Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
         ImGui::Checkbox("Enabled", &effects.enabled);

         effects.color_quality = color_quality_from_string(ImGui::StringPicker(
            "Color Quality", std::string{to_string(effects.color_quality)},
            std::initializer_list<std::string>{to_string(Color_quality::normal),
                                               to_string(Color_quality::high),
                                               to_string(Color_quality::ultra)}));

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
               "Does not have any effect when HDR Rendering is enabled.");
         }

         ImGui::Checkbox("Bloom", &effects.bloom);
         ImGui::Checkbox("Vignette", &effects.vignette);
         ImGui::Checkbox("Film Grain", &effects.film_grain);
         ImGui::Checkbox("Allow Colored Film Grain", &effects.colored_film_grain);
      }

      if (ImGui::CollapsingHeader("Developer")) {
         ImGui::Checkbox("Allow Event Queries", &developer.allow_event_queries);
      }

      ImGui::End();
   }

private:
   void parse_file(const std::string& path);
};

extern User_config user_config;

}
