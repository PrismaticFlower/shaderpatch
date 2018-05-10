
#include "control.hpp"
#include "../logger.hpp"
#include "file_dialogs.hpp"

#include <fstream>
#include <string_view>

#include <imgui.h>

namespace sp::effects {

using namespace std::literals;
namespace fs = std::filesystem;

void Control::show_imgui(HWND game_window) noexcept
{
   color_grading.show_imgui();
   bloom.show_imgui();

   ImGui::Begin("Effects Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::Checkbox("Enable Effects", &_enabled);

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Device must be reset before this will take efect.");
   }

   ImGui::Separator();

   if (ImGui::Button("Open Config")) {
      if (auto path = win32::open_file_dialog({{L"Effects Config", L"*.spfx"}},
                                              game_window, fs::current_path(),
                                              L"mod_config.spfx"s);
          path) {
         load_params_from_yaml_file(*path);
      }
   }

   if (_open_failure) {
      ImGui::SameLine();
      ImGui::TextColored({1.0f, 0.2f, 0.33f, 1.0f}, "Open Failed!");
   }

   ImGui::SameLine();

   if (ImGui::Button("Save Config")) {
      if (auto path = win32::save_file_dialog({{L"Effects Config", L"*.spfx"}},
                                              game_window, fs::current_path(),
                                              L"mod_config.spfx"s);
          path) {
         save_params_to_yaml_file(*path);
      }
   }

   if (_save_failure) {
      ImGui::SameLine();
      ImGui::TextColored({1.0f, 0.2f, 0.33f, 1.0f}, "Save Failed!");
   }

   ImGui::End();
}

void Control::drop_device_resources() noexcept
{
   color_grading.drop_device_resources();
   bloom.drop_device_resources();
}

void Control::read_config(YAML::Node config)
{
   color_grading.params(config["ColorGrading"s].as<Color_grading_params>());
   bloom.params(config["Bloom"s].as<Bloom_params>());
}

void Control::save_params_to_yaml_file(const fs::path& save_to) noexcept
{
   YAML::Node config;

   config["ColorGrading"s] = color_grading.params();
   config["Bloom"s] = bloom.params();

   std::ofstream file{save_to};

   if (!file) {
      log(Log_level::error, "Failed to open file "sv, save_to,
          " to write colour grading config."sv);

      _save_failure = true;

      return;
   }

   file << "# Auto-Generated Effects Config. May have less than ideal formatting.\n"sv;

   file << config;

   _save_failure = false;
}

void Control::load_params_from_yaml_file(const fs::path& load_from) noexcept
{
   YAML::Node config;

   try {
      config = YAML::LoadFile(load_from.string());
   }
   catch (std::exception& e) {
      log(Log_level::error, "Failed to open file "sv, load_from,
          " to read colour grading config. Reason: "sv, e.what());

      _open_failure = true;
   }

   try {
      read_config(config);
   }
   catch (std::exception& e) {
      log(Log_level::warning,
          "Exception occured while reading colour grading config from "sv,
          load_from, " Reason: "sv, e.what());

      _open_failure = true;
   }

   _open_failure = false;
}

}
