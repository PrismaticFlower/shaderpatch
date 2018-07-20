#pragma once

#include "imgui_ext.hpp"
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

enum class Post_aa_quality { fastest, fast, slower, slowest };

inline std::string to_string(Post_aa_quality quality) noexcept
{
   using namespace std::literals;

   switch (quality) {
   case Post_aa_quality::fastest:
      return "fastest"s;
   case Post_aa_quality::fast:
      return "fast"s;
   case Post_aa_quality::slower:
      return "slower"s;
   case Post_aa_quality::slowest:
      return "slowest"s;
   }

   std::terminate();
}

inline Post_aa_quality aa_quality_from_string(std::string_view string) noexcept
{
   if (string == to_string(Post_aa_quality::fastest)) {
      return Post_aa_quality::fastest;
   }
   else if (string == to_string(Post_aa_quality::fast)) {
      return Post_aa_quality::fast;
   }
   else if (string == to_string(Post_aa_quality::slower)) {
      return Post_aa_quality::slower;
   }
   else if (string == to_string(Post_aa_quality::slowest)) {
      return Post_aa_quality::slowest;
   }
   else {
      return Post_aa_quality::fastest;
   }
}

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
      bool advertise_presence = true;
      bool force_anisotropic_filtering = true;
      bool smooth_bloom = true;
      bool gaussian_blur_blur_particles = true;
      bool gaussian_scene_blur = true;

      int reflection_buffer_factor = 1;
      int refraction_buffer_factor = 2;

      int anisotropic_filtering = 1;

      Post_aa_quality post_aa_quality = Post_aa_quality::fastest;
   } rendering;

   struct {
      bool unlock_fps = true;
   } game;

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

         ImGui::Checkbox("Smooth Bloom", &rendering.smooth_bloom);

         ImGui::Checkbox("Gaussian Blur Blur Particles",
                         &rendering.gaussian_blur_blur_particles);

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Trust me, it's actually well named.");
         }

         ImGui::Checkbox("Gaussian Scene Blur", &rendering.gaussian_scene_blur);

         ImGui::Checkbox("Advertise Presence", &rendering.advertise_presence);

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Will not take effect on Device Reset.");
         }

         rendering.post_aa_quality = aa_quality_from_string(ImGui::StringPicker(
            "Post AA Quality", std::string{to_string(rendering.post_aa_quality)},
            std::initializer_list<std::string>{to_string(Post_aa_quality::fastest),
                                               to_string(Post_aa_quality::fast),
                                               to_string(Post_aa_quality::slower),
                                               to_string(Post_aa_quality::slowest)}));

         ImGui::Checkbox("Force Anisotropic Filtering",
                         &rendering.force_anisotropic_filtering);

         ImGui::DragInt("Reflection Buffer Factor",
                        &rendering.reflection_buffer_factor, 1, 1, 32);

         ImGui::DragInt("Refraction Buffer Factor",
                        &rendering.refraction_buffer_factor, 1, 1, 32);

         ImGui::DragInt("Anisotropic Filtering",
                        &rendering.anisotropic_filtering, 1, 1, 16);
      }

      if (ImGui::CollapsingHeader("Game")) {
         ImGui::Checkbox("Unlock FPS", &game.unlock_fps);

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Will not take effect on Device Reset.");
         }
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

      rendering.smooth_bloom = config["Rendering"s]["SmoothBloom"s].as<bool>();

      rendering.gaussian_blur_blur_particles =
         config["Rendering"s]["GaussianBlurBlurParticles"s].as<bool>();

      rendering.gaussian_scene_blur =
         config["Rendering"s]["GaussianSceneBlur"s].as<bool>();

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

      rendering.post_aa_quality = aa_quality_from_string(
         config["Rendering"s]["PostAAQuality"s].as<std::string>());

      rendering.force_anisotropic_filtering =
         config["Rendering"s]["ForceAnisotropicFiltering"s].as<bool>();

      rendering.advertise_presence =
         config["Rendering"s]["AdvertisePresence"s].as<bool>();

      game.unlock_fps = config["Game"s]["UnlockFPS"s].as<bool>();

      developer.toggle_key = config["Developer"s]["ScreenToggle"s].as<int>();
   }
};

}
