#pragma once

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

struct User_config {
   User_config() = default;

   explicit User_config(const std::string& path) noexcept
   {
      using namespace std::literals;

      try {
         parse_file(path);
      }
      catch (std::exception& e) {
         log(Log_level::warning, "Failed to read config file "sv,
             std::quoted(path), ". reason:"sv, e.what());
      }
   }

   struct {
      bool enabled = false;
      bool windowed = false;
      glm::ivec2 resolution{800, 600};

      bool borderless = true;
   } display;

   struct {
      bool high_res_reflections = true;
      bool custom_materials = true;

      int reflection_buffer_factor = 1;
      int refraction_buffer_factor = 2;

      int anisotropic_filtering = 1;
   } rendering;

   struct {
      std::uintptr_t toggle_key{0};
   } developer;

   void show_imgui(bool* apply = nullptr) noexcept
   {
      ImGui::Begin("User Config", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

      if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
         ImGui::Checkbox("Override Enabled", &display.enabled);
         ImGui::SameLine();
         ImGui::Checkbox("Windowed", &display.windowed);

         ImGui::InputInt2("Resolution", &display.resolution.x);
         ImGui::SameLine();
         ImGui::Checkbox("Borderless", &display.borderless);
      }

      if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
         ImGui::Checkbox("High Resolution Reflections", &rendering.high_res_reflections);
         ImGui::Checkbox("Custom Materials", &rendering.custom_materials);

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Will not take effect on Apply.");
         }

         ImGui::DragInt("Reflection Buffer Factor",
                        &rendering.reflection_buffer_factor, 1, 1, 32);

         ImGui::DragInt("Refraction Buffer Factor",
                        &rendering.refraction_buffer_factor, 1, 1, 32);

         ImGui::DragInt("Anisotropic Filtering",
                        &rendering.anisotropic_filtering, 1, 1, 16);
      }

      if (apply) {
         ImGui::Separator();

         *apply = ImGui::Button("Reset Device & Apply");
      }

      ImGui::End();
   }

private:
   void parse_file(const std::string& path)
   {
      using namespace std::literals;

      const auto config = YAML::LoadFile(path);

      display.enabled = config["Display"s]["Enabled"s].as<bool>();
      display.windowed = config["Display"s]["Windowed"s].as<bool>();
      display.windowed = config["Display"s]["Borderless"s].as<bool>();
      display.resolution.x = config["Display"s]["Resolution"s][0].as<int>();
      display.resolution.y = config["Display"s]["Resolution"s][1].as<int>();

      rendering.high_res_reflections =
         config["Rendering"s]["HighResolutionReflections"s].as<bool>();

      const auto reflection_scale =
         config["Rendering"s]["ReflectionBufferScale"s].as<double>();

      rendering.reflection_buffer_factor =
         static_cast<int>(1.0 / glm::clamp(reflection_scale, 0.01, 1.0));

      const auto refraction_scale =
         config["Rendering"s]["RefractionBufferScale"s].as<double>();

      rendering.refraction_buffer_factor =
         static_cast<int>(1.0 / glm::clamp(refraction_scale, 0.01, 1.0));

      rendering.anisotropic_filtering =
         config["Rendering"s]["AnisotropicFiltering"s].as<int>();

      rendering.custom_materials =
         config["Rendering"s]["CustomMaterials"s].as<bool>();

      developer.toggle_key = config["Developer"s]["ScreenToggle"s].as<int>();
   }
};
}
