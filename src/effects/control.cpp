
#include "control.hpp"
#include "../logger.hpp"
#include "file_dialogs.hpp"
#include "imgui_ext.hpp"
#include "imgui_tabs.h"
#include "volume_resource.hpp"

#include <fstream>
#include <sstream>
#include <string_view>

#include <imgui.h>

namespace sp::effects {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

Bloom_params show_bloom_imgui(Bloom_params params) noexcept;

Vignette_params show_vignette_imgui(Vignette_params params) noexcept;

Color_grading_params show_color_grading_imgui(Color_grading_params params) noexcept;

Film_grain_params show_film_grain_imgui(Film_grain_params params) noexcept;

}

void Control::show_imgui(HWND game_window) noexcept
{
   ImGui::Begin("Effects", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

   ImGui::BeginTabBar("Effects Config", ImGuiTabBarFlags_SizingPolicyEqual |
                                           ImGuiTabBarFlags_NoSelectionOnAppearing);

   static bool first_open = true;
   if (ImGui::TabItem("Control", nullptr,
                      std::exchange(first_open, false) ? ImGuiTabItemFlags_SetSelected
                                                       : ImGuiTabItemFlags_None)) {
      ImGui::Text("Device must be reset for effects control settings to be "
                  "applied correctly!");

      ImGui::Checkbox("Enable Effects", &_enabled);

      if (ImGui::CollapsingHeader("Effects Config", ImGuiTreeNodeFlags_DefaultOpen)) {
         ImGui::Checkbox("HDR Rendering", &_config.hdr_rendering);
      }

      ImGui::Separator();

      if (ImGui::Button("Open Config")) {
         if (auto path =
                win32::open_file_dialog({{L"Effects Config", L"*.spfx"}}, game_window,
                                        fs::current_path(), L"mod_config.spfx"s);
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
         if (auto path =
                win32::save_file_dialog({{L"Effects Config", L"*.spfx"}}, game_window,
                                        fs::current_path(), L"mod_config.spfx"s);
             path) {
            save_params_to_yaml_file(*path);
         }
      }

      ImGui::SameLine();

      if (ImGui::Button("Save Munged Config")) {
         if (auto path = win32::save_file_dialog({{L"Munged Effects Config", L"*.mspfx"}},
                                                 game_window, fs::current_path(),
                                                 L"mod_config.mspfx"s);
             path) {
            save_params_to_munged_file(*path);
         }
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip(
            "Save out a munged config to be loaded from a map script. Keep in "
            "mind Shader "
            "Patch can not reload these files from the debug screen.");
      }

      if (_save_failure) {
         ImGui::SameLine();
         ImGui::TextColored({1.0f, 0.2f, 0.33f, 1.0f}, "Save Failed!");
      }
   }

   if (ImGui::TabItem("Color Grading")) {
      postprocess.color_grading_params(
         show_color_grading_imgui(postprocess.color_grading_params()));
   }

   if (ImGui::TabItem("Bloom")) {
      postprocess.bloom_params(show_bloom_imgui(postprocess.bloom_params()));
   }

   if (ImGui::TabItem("Vignette")) {
      postprocess.vignette_params(show_vignette_imgui(postprocess.vignette_params()));
   }

   if (ImGui::TabItem("Film Grain")) {
      postprocess.film_grain_params(
         show_film_grain_imgui(postprocess.film_grain_params()));
   }

   ImGui::EndTabBar();
   ImGui::End();
}

void Control::drop_device_resources() noexcept
{
   postprocess.drop_device_resources();
   scene_blur.drop_device_resources();
}

void Control::read_config(YAML::Node config)
{
   this->config(
      config["Control"s].as<Effects_control_config>(Effects_control_config{}));
   postprocess.color_grading_params(
      config["ColorGrading"s].as<Color_grading_params>(Color_grading_params{}));
   postprocess.bloom_params(config["Bloom"s].as<Bloom_params>(Bloom_params{}));
   postprocess.vignette_params(
      config["Vignette"s].as<Vignette_params>(Vignette_params{}));
   postprocess.film_grain_params(
      config["FilmGrain"s].as<Film_grain_params>(Film_grain_params{}));
}

auto Control::output_params_to_yaml_string() noexcept -> std::string
{
   YAML::Node config;

   config["Control"s] = _config;
   config["ColorGrading"s] = postprocess.color_grading_params();
   config["Bloom"s] = postprocess.bloom_params();
   config["Vignette"s] = postprocess.vignette_params();
   config["FilmGrain"s] = postprocess.film_grain_params();

   std::stringstream stream;
   ;
   stream << "# Auto-Generated Effects Config. May have less than ideal formatting.\n"sv;

   stream << config;

   return stream.str();
}

void Control::save_params_to_yaml_file(const fs::path& save_to) noexcept
{
   auto config = output_params_to_yaml_string();

   std::ofstream file{save_to};

   if (!file) {
      log(Log_level::error, "Failed to open file "sv, save_to,
          " to write effects config."sv);

      _save_failure = true;

      return;
   }

   file << config;

   _save_failure = false;
}

void Control::save_params_to_munged_file(const fs::path& save_to) noexcept
{
   auto config = output_params_to_yaml_string();

   try {
      save_volume_resource(save_to.u8string(), save_to.stem().u8string(),
                           Volume_resource_type::fx_config,
                           gsl::span{reinterpret_cast<std::byte*>(config.data()),
                                     gsl::narrow_cast<gsl::index>(config.size())});

      _save_failure = false;
   }
   catch (std::exception& e) {
      log(Log_level::warning, "Exception occured while writing effects config tp "sv,
          save_to, " Reason: "sv, e.what());

      _save_failure = true;
   }
}

void Control::load_params_from_yaml_file(const fs::path& load_from) noexcept
{
   YAML::Node config;

   try {
      config = YAML::LoadFile(load_from.string());
   }
   catch (std::exception& e) {
      log(Log_level::error, "Failed to open file "sv, load_from,
          " to read effects config. Reason: "sv, e.what());

      _open_failure = true;
   }

   try {
      read_config(config);
   }
   catch (std::exception& e) {
      log(Log_level::warning, "Exception occured while reading effects config from "sv,
          load_from, " Reason: "sv, e.what());

      _open_failure = true;
   }

   _open_failure = false;
}

namespace {

Bloom_params show_bloom_imgui(Bloom_params params) noexcept
{
   const auto max_float = std::numeric_limits<float>::max();

   if (ImGui::CollapsingHeader("Basic Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Enabled", &params.enabled);

      ImGui::DragFloat("Threshold", &params.threshold, 0.025f);
      ImGui::DragFloat("Intensity", &params.intensity, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::ColorEdit3("Tint", &params.tint.x, ImGuiColorEditFlags_Float);
   }

   if (ImGui::CollapsingHeader("Individual Scales & Tints")) {
      ImGui::DragFloat("Inner Scale", &params.inner_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Inner Mid Scale", &params.inner_mid_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Mid Scale", &params.mid_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Outer Mid Scale", &params.outer_mid_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::DragFloat("Outer Scale", &params.outer_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());

      ImGui::ColorEdit3("Inner Tint", &params.inner_tint.x, ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Inner Mid Tint", &params.inner_mid_tint.x,
                        ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Mid Tint", &params.mid_tint.x, ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Outer Mid Tint", &params.outer_mid_tint.x,
                        ImGuiColorEditFlags_Float);
      ImGui::ColorEdit3("Outer Tint", &params.outer_tint.x, ImGuiColorEditFlags_Float);
   }

   if (ImGui::CollapsingHeader("Dirt")) {
      ImGui::Checkbox("Use Dirt", &params.use_dirt);
      ImGui::DragFloat("Dirt Scale", &params.dirt_scale, 0.025f, 0.0f,
                       std::numeric_limits<float>::max());
      ImGui::ColorEdit3("Dirt Tint", &params.dirt_tint.x, ImGuiColorEditFlags_Float);

      ImGui::InputText("Dirt Texture", params.dirt_texture_name);
   }

   ImGui::Separator();

   if (ImGui::Button("Reset Settings")) params = Bloom_params{};

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Reset bloom params to default settings.");
   }

   return params;
}

Vignette_params show_vignette_imgui(Vignette_params params) noexcept
{
   ImGui::Checkbox("Enabled", &params.enabled);

   ImGui::DragFloat("End", &params.end, 0.05f, 0.0f, 2.0f);
   ImGui::DragFloat("Start", &params.start, 0.05f, 0.0f, 2.0f);

   return params;
}

Color_grading_params show_color_grading_imgui(Color_grading_params params) noexcept
{
   if (ImGui::CollapsingHeader("Basic Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::ColorEdit3("Colour Filter", &params.color_filter.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Exposure", &params.exposure, 0.01f);
      ImGui::DragFloat("Brightness", &params.brightness, 0.01f);
      ImGui::DragFloat("Saturation", &params.saturation, 0.01f, 0.0f, 5.0f);
      ImGui::DragFloat("Contrast", &params.contrast, 0.01f, 0.01f, 5.0f);
   }

   if (ImGui::CollapsingHeader("Filmic Tonemapping")) {
      // filmic::show_imgui_curve(params);

      ImGui::DragFloat("Toe Strength", &params.filmic_toe_strength, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Toe Length", &params.filmic_toe_length, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Strength", &params.filmic_shoulder_strength,
                       0.01f, 0.0f, 100.0f);
      ImGui::DragFloat("Shoulder Length", &params.filmic_shoulder_length, 0.01f,
                       0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Angle", &params.filmic_shoulder_angle, 0.01f,
                       0.0f, 1.0f);

      if (ImGui::Button("Reset To Linear")) {
         params.filmic_toe_strength = 0.0f;
         params.filmic_toe_length = 0.5f;
         params.filmic_shoulder_strength = 0.0f;
         params.filmic_shoulder_length = 0.5f;
         params.filmic_shoulder_angle = 0.0f;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip("Reset the curve to linear values.");
      }

      ImGui::SameLine();

      if (ImGui::Button("Reset To Example")) {
         params.filmic_toe_strength = 0.5f;
         params.filmic_toe_length = 0.5f;
         params.filmic_shoulder_strength = 2.0f;
         params.filmic_shoulder_length = 0.5f;
         params.filmic_shoulder_angle = 1.0f;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip("Reset the curve to example starting point values.");
      }
   }

   if (ImGui::CollapsingHeader("Lift / Gamma / Gain")) {
      ImGui::ColorEdit3("Shadow Colour", &params.shadow_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Shadow Offset", &params.shadow_offset, 0.005f);

      ImGui::ColorEdit3("Midtone Colour", &params.midtone_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Midtone Offset", &params.midtone_offset, 0.005f);

      ImGui::ColorEdit3("Hightlight Colour", &params.highlight_color.x,
                        ImGuiColorEditFlags_Float);
      ImGui::DragFloat("Hightlight Offset", &params.highlight_offset, 0.005f);
   }

   ImGui::Separator();

   if (ImGui::Button("Reset Settings")) {
      params = Color_grading_params{};
   }

   return params;
}

Film_grain_params show_film_grain_imgui(Film_grain_params params) noexcept
{
   ImGui::Checkbox("Enabled", &params.enabled);
   ImGui::Checkbox("Colored", &params.colored);

   ImGui::DragFloat("Amount", &params.amount, 0.001f, 0.0f, 1.0f);
   ImGui::DragFloat("Size", &params.size, 0.05f, 1.6f, 3.0f);
   ImGui::DragFloat("Color Amount", &params.color_amount, 0.05f, 0.0f, 1.0f);
   ImGui::DragFloat("Luma Amount", &params.luma_amount, 0.05f, 0.0f, 1.0f);

   return params;
}

}
}
