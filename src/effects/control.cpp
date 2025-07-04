
#include "control.hpp"
#include "../imgui/imgui_ext.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"
#include "file_dialogs.hpp"
#include "filmic_tonemapper.hpp"
#include "postprocess_params.hpp"
#include "tonemappers.hpp"
#include "volume_resource.hpp"

#include <fstream>
#include <functional>
#include <numbers>
#include <sstream>
#include <string_view>

#include <fmt/format.h>

#include "../imgui/imgui.h"

namespace sp::effects {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

constexpr std::wstring_view auto_user_config_name = L"shader patch.spfx";

Bloom_params show_bloom_imgui(Bloom_params params) noexcept;

Vignette_params show_vignette_imgui(Vignette_params params) noexcept;

Color_grading_params show_color_grading_imgui(Color_grading_params params) noexcept;

Color_grading_params show_tonemapping_imgui(Color_grading_params params) noexcept;

Film_grain_params show_film_grain_imgui(Film_grain_params params) noexcept;

DOF_params show_dof_imgui(DOF_params params) noexcept;

SSAO_params show_ssao_imgui(SSAO_params params) noexcept;

FFX_cas_params show_ffx_cas_imgui(FFX_cas_params params) noexcept;

void show_tonemapping_curve(std::function<float(float)> tonemapper) noexcept;
}

Control::Control(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept
   : postprocess{device, shaders},
     cmaa2{device, shaders.compute("CMAA2"sv)},
     ssao{device, shaders},
     ffx_cas{device, shaders},
     mask_nan{device, shaders},
     profiler{device}
{
   if (user_config.graphics.enable_user_effects_config) {
      load_params_from_yaml_file(user_config.graphics.user_effects_config);
   }
   else if (user_config.graphics.enable_user_effects_auto_config) {
      try {
         _has_auto_user_config =
            std::filesystem::exists(auto_user_config_name) &&
            std::filesystem::is_regular_file(auto_user_config_name);

         if (_has_auto_user_config) {
            load_params_from_yaml_file(auto_user_config_name);
         }
      }
      catch (std::exception&) {
         _has_auto_user_config = false;
      }
   }
}

bool Control::enabled(const bool enable) noexcept
{
   _enabled = enable;

   if (!_enabled && user_config.graphics.enable_user_effects_config)
      load_params_from_yaml_file(user_config.graphics.user_effects_config);
   else if (!_enabled && _has_auto_user_config)
      load_params_from_yaml_file(auto_user_config_name);

   return enabled();
}

bool Control::enabled() const noexcept
{
   const bool enable_user_effects_auto_config =
      _has_auto_user_config && user_config.graphics.enable_user_effects_auto_config;

   return (_enabled || user_config.graphics.enable_user_effects_config ||
           enable_user_effects_auto_config);
}

bool Control::allow_scene_blur() const noexcept
{
   if (!enabled()) return true;

   const Bloom_params& params = postprocess.bloom_params();

   return !(enabled() && params.mode == Bloom_mode::blended);
}

void Control::show_imgui(HWND game_window) noexcept
{
   ImGui::SetNextWindowSize({533, 591}, ImGuiCond_FirstUseEver);
   ImGui::Begin("Effects", nullptr);

   if (ImGui::BeginTabBar("Effects Config")) {
      if (ImGui::BeginTabItem("Control")) {
         ImGui::BeginDisabled(enabled() && !_enabled);

         ImGui::Checkbox("Enable Effects", &_enabled);

         ImGui::EndDisabled();

         if (enabled() && !_enabled) {
            ImGui::Text("Effects are being enabled from the user config.");
         }

         if (ImGui::CollapsingHeader("Effects Config", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("HDR Rendering", &_config.hdr_rendering);

            if (ImGui::IsItemHovered()) {
               ImGui::SetTooltip(
                  "HDR rendering works best with custom materials "
                  "and may give poor results without them.");
            }

            ImGui::Checkbox("Request Order-Independent Transparency",
                            &_config.oit_requested);

            if (ImGui::IsItemHovered()) {
               ImGui::SetTooltip(
                  "Informs SP that OIT is required for some models to "
                  "render correctly and that it should be enabled if the "
                  "user's GPU supports it.");
            }

            ImGui::Checkbox("Request Soft Skinning", &_config.soft_skinning_requested);

            if (ImGui::IsItemHovered()) {
               ImGui::SetTooltip(
                  "Informs SP that soft skinning is required for some models "
                  "to render correctly and that it should be enabled even if "
                  "the "
                  "user has switched it off.");
            }

            if (!_config.hdr_rendering) {
               ImGui::Checkbox("Floating-point Render Targets",
                               &_config.fp_rendertargets);

               if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip("Controls usage of floating-point "
                                    "rendertargets, which can preserve more "
                                    "color detail in the bright areas of the "
                                    "scene for when Bloom is applied.");
               }

               ImGui::Checkbox("Disable Light Brightness Rescaling",
                               &_config.disable_light_brightness_rescaling);

               if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(
                     "Disable light brightness rescaling in stock shaders. Has "
                     "no affect on custom materials. HDR Rendering implies "
                     "this option.");
               }
            }

            if (_config.hdr_rendering || _config.fp_rendertargets) {
               ImGui::Checkbox("Bugged Cloth Workaround",
                               &_config.workaround_bugged_cloth);

               if (ImGui::IsItemHovered()) {
                  ImGui::SetTooltip(
                     "Some cloth can produce NaNs when drawn. These turn into "
                     "large black boxes when ran through the bloom filter. "
                     "This option enables a pass to convert these NaNs into "
                     "pure black pixels.\n\nThis option should not be "
                     "prohbitively expensive but it is always cheaper to not "
                     "run it if it is not needed.\n\nIf you're a modder always "
                     "try and fix your cloth assets first before enabling "
                     "this.");
               }
            }
         }

         ImGui::Checkbox("Profiler Enabled", &profiler.enabled);

         ImGui::Separator();

         imgui_save_widget(game_window);

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Post Processing")) {
         show_post_processing_imgui();

         ImGui::Separator();

         imgui_save_widget(game_window);

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Color Grading Regions")) {
         postprocess.show_color_grading_regions_imgui(game_window,
                                                      &show_color_grading_imgui,
                                                      &show_bloom_imgui);

         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }

   ImGui::Separator();

   ImGui::End();

   config_changed();
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
   postprocess.dof_params(config["DOF"s].as<DOF_params>(DOF_params{}));
   ssao.params(config["SSAO"s].as<SSAO_params>(SSAO_params{false}));
   ffx_cas.params(config["ContrastAdaptiveSharpening"s].as<FFX_cas_params>(
      FFX_cas_params{false}));
}

auto Control::output_params_to_yaml_string() noexcept -> std::string
{
   YAML::Node config;

   config["Control"s] = _config;
   config["ColorGrading"s] = postprocess.color_grading_params();
   config["Bloom"s] = postprocess.bloom_params();
   config["Vignette"s] = postprocess.vignette_params();
   config["FilmGrain"s] = postprocess.film_grain_params();
   config["DOF"s] = postprocess.dof_params();
   config["SSAO"s] = ssao.params();
   config["ContrastAdaptiveSharpening"s] = ffx_cas.params();

   std::stringstream stream;

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
      save_volume_resource(save_to, save_to.stem().string(),
                           Volume_resource_type::fx_config,
                           std::span{reinterpret_cast<std::byte*>(config.data()),
                                     config.size()});
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

void Control::imgui_save_widget(const HWND game_window) noexcept
{
   if (ImGui::Button("Open Config")) {
      if (auto path = win32::open_file_dialog({{L"Effects Config", L"*.spfx"}},
                                              game_window, fs::current_path(),
                                              L"mod_config.spfx"s);
          path) {
         load_params_from_yaml_file(*path);
      }
   }

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Open a previously saved config.");
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

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Save out a config to be passed to `spfx_munge` or "
                        "loaded back up from the developer screen.");
   }

   ImGui::SameLine();

   if (ImGui::Button("Save Munged Config")) {
      if (auto path =
             win32::save_file_dialog({{L"Munged Effects Config", L"*.mspfx"}}, game_window,
                                     fs::current_path(), L"mod_config.mspfx"s);
          path) {
         save_params_to_munged_file(*path);
      }
   }

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
         "Save out a munged config to be loaded from a map script. Keep "
         "in "
         "mind Shader "
         "Patch can not reload these files from the developer screen.");
   }

   if (_save_failure) {
      ImGui::SameLine();
      ImGui::TextColored({1.0f, 0.2f, 0.33f, 1.0f}, "Save Failed!");
   }
}

void Control::show_post_processing_imgui() noexcept
{
   if (ImGui::BeginTabBar("Post Processing", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("Color Grading")) {
         postprocess.color_grading_params(
            show_color_grading_imgui(postprocess.color_grading_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Tonemapping")) {
         postprocess.color_grading_params(
            show_tonemapping_imgui(postprocess.color_grading_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Bloom")) {
         postprocess.bloom_params(show_bloom_imgui(postprocess.bloom_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Vignette")) {
         postprocess.vignette_params(
            show_vignette_imgui(postprocess.vignette_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Film Grain")) {
         postprocess.film_grain_params(
            show_film_grain_imgui(postprocess.film_grain_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Depth of Field")) {
         postprocess.dof_params(show_dof_imgui(postprocess.dof_params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("SSAO")) {
         ssao.params(show_ssao_imgui(ssao.params()));

         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Contrast Adaptive Sharpening")) {
         ffx_cas.params(show_ffx_cas_imgui(ffx_cas.params()));

         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }
}

void Control::config_changed() noexcept
{
   postprocess.hdr_state(_config.hdr_rendering ? Hdr_state::hdr : Hdr_state::stock);
}

namespace {

Bloom_params show_bloom_imgui(Bloom_params params) noexcept
{
   if (ImGui::CollapsingHeader("Basic Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Enabled", &params.enabled);

      const ImVec2 pre_mode_cursor = ImGui::GetCursorPos();

      if (ImGui::RadioButton("Blended", params.mode == Bloom_mode::blended)) {
         params.mode = Bloom_mode::blended;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip(
            "New blended bloom mode. It doesn't require configuring a "
            "threshold and is easier to work with.\n\nHowever it can result in "
            "everything having a very slightly \"softer\" look to it, "
            "depending on how high the Blend Factor is.");
      }

      ImGui::SameLine();

      if (ImGui::RadioButton("Threshold##Mode", params.mode == Bloom_mode::threshold)) {
         params.mode = Bloom_mode::threshold;
      }

      if (ImGui::IsItemHovered()) {
         ImGui::SetTooltip(
            "Classic threshold bloom mode. What has been in SP "
            "since v1.0, it can give nice results as well but "
            "can require more tweaking to get right.\n\nHowever if you don't "
            "like the softness of the blended mode and lowering the Blend "
            "Factor doesn't help but you still want bloom this mode is likely "
            "what you want.");
      }

      ImGui::SetCursorPos(pre_mode_cursor);

      ImGui::LabelText("Mode", "");

      if (params.mode == Bloom_mode::blended)
         ImGui::DragFloat("Blend Factor", &params.blend_factor, 0.025f);
      else
         ImGui::DragFloat("Threshold##Param", &params.threshold, 0.025f);

      params.blend_factor = std::clamp(params.blend_factor, 0.0f, 1.0f);
      params.threshold = std::clamp(params.threshold, 0.0f, 1.0f);

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

   if (ImGui::CollapsingHeader("Channel Mixer")) {
      ImGui::DragFloatFormatted3("Red", &params.channel_mix_red.x,
                                 {"R: %.3f", "G: %.3f", "B: %.3f"}, 0.025f,
                                 -2.0f, 2.0f);
      ImGui::DragFloatFormatted3("Green", &params.channel_mix_green.x,
                                 {"R: %.3f", "G: %.3f", "B: %.3f"}, 0.025f,
                                 -2.0f, 2.0f);
      ImGui::DragFloatFormatted3("Blue", &params.channel_mix_blue.x,
                                 {"R: %.3f", "G: %.3f", "B: %.3f"}, 0.025f,
                                 -2.0f, 2.0f);
   }

   if (ImGui::CollapsingHeader("Hue / Saturation / Value")) {
      params.hsv_hue_adjustment *= 360.0f;

      ImGui::DragFloat("Hue Adjustment", &params.hsv_hue_adjustment, 1.0f,
                       -180.0f, 180.0f);

      ImGui::DragFloat("Saturation Adjustment",
                       &params.hsv_saturation_adjustment, 0.025f, 0.0f, 2.0f);

      ImGui::DragFloat("Value Adjustment", &params.hsv_value_adjustment, 0.025f,
                       0.0f, 2.0f);

      params.hsv_hue_adjustment /= 360.0f;
   }

   ImGui::Separator();

   if (ImGui::Button("Reset Settings")) params = Color_grading_params{};

   if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Will also reset Tonemapping settings.");

   return params;
}

Color_grading_params show_tonemapping_imgui(Color_grading_params params) noexcept
{
   params.tonemapper = tonemapper_from_string(ImGui::StringPicker(
      "Tonemapper", std::string{to_string(params.tonemapper)},
      std::initializer_list<std::string>{to_string(Tonemapper::filmic),
                                         to_string(Tonemapper::aces_fitted),
                                         to_string(Tonemapper::filmic_heji2015),
                                         to_string(Tonemapper::reinhard),
                                         to_string(Tonemapper::none)}));

   if (params.tonemapper == Tonemapper::filmic) {
      show_tonemapping_curve([curve = filmic::color_grading_params_to_curve(params)](
                                const float v) { return filmic::eval(v, curve); });

      ImGui::DragFloat("Toe Strength", &params.filmic_toe_strength, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Toe Length", &params.filmic_toe_length, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Strength", &params.filmic_shoulder_strength,
                       0.01f, 0.0f, 100.0f);
      ImGui::DragFloat("Shoulder Length", &params.filmic_shoulder_length, 0.01f,
                       0.0f, 1.0f);
      ImGui::DragFloat("Shoulder Angle", &params.filmic_shoulder_angle, 0.01f,
                       0.0f, 1.0f);

      if (ImGui::Button("Reset to Linear")) {
         params.filmic_toe_strength = 0.0f;
         params.filmic_toe_length = 0.5f;
         params.filmic_shoulder_strength = 0.0f;
         params.filmic_shoulder_length = 0.5f;
         params.filmic_shoulder_angle = 0.0f;
      }

      if (ImGui::IsItemHovered())
         ImGui::SetTooltip("Reset the curve to linear values.");

      ImGui::SameLine();

      if (ImGui::Button("Load Example Starting Point")) {
         params.filmic_toe_strength = 0.5f;
         params.filmic_toe_length = 0.5f;
         params.filmic_shoulder_strength = 2.0f;
         params.filmic_shoulder_length = 0.5f;
         params.filmic_shoulder_angle = 1.0f;
      }
   }
   else if (params.tonemapper == Tonemapper::aces_fitted) {
      show_tonemapping_curve(
         [](const float v) { return eval_aces_srgb_fitted(glm::vec3{v}).r; });
   }
   else if (params.tonemapper == Tonemapper::filmic_heji2015) {
      show_tonemapping_curve([whitepoint = params.filmic_heji_whitepoint](const float v) {
         return eval_filmic_hejl2015(glm::vec3{v}, whitepoint).x;
      });

      ImGui::DragFloat("Whitepoint", &params.filmic_heji_whitepoint, 0.01f);
   }
   else if (params.tonemapper == Tonemapper::reinhard) {
      show_tonemapping_curve(
         [](const float v) { return eval_reinhard(glm::vec3{v}).x; });
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

DOF_params show_dof_imgui(DOF_params params) noexcept
{
   ImGui::Checkbox("Enabled", &params.enabled);

   ImGui::DragFloat("Film Size", &params.film_size_mm, 1.0f, 1.0f, 256.0f);

   ImGui::SetItemTooltip(
      "Film/Sensor Size for the Depth of Field. Due to limitations in how "
      "Shader Patch works this does not alter the FOV.");

   ImGui::DragFloat("Focus Distance", &params.focus_distance, 1.0f, 0.0f, 1e10f);

   ImGui::SetItemTooltip("Distance to the plane in focus.");

   int f_stop_index = static_cast<int>(std::round(
      std::log(double{params.f_stop}) / std::log(std::numbers::sqrt2_v<double>)));

   if (ImGui::SliderInt("f-stop", &f_stop_index, 0, 10,
                        fmt::format("f/{:.1f}", params.f_stop).c_str(),
                        ImGuiSliderFlags_NoInput)) {
      params.f_stop = static_cast<float>(
         std::pow(std::numbers::sqrt2_v<double>, static_cast<double>(f_stop_index)));
   }

   ImGui::SetItemTooltip(
      "f-stop/f-number for the lens. Higher numbers create "
      "less blur, lower numbers cause more blur. This does not currently alter "
      "the exposure either as it would on a real lens.");

   ImGui::InputFloat("f-stop##raw", &params.f_stop, 1.0f, 1.0f, "f/%.1f");

   ImGui::SetItemTooltip("Manual input for the f-stop.");

   ImGui::Separator();

   ImGui::TextWrapped(
      "Focal Length is controlled by ingame FOV.\n\nBe aware as well that the "
      "current Depth of Field implementation can interact poorly with "
      "transparent surfaces/particles and also the far scene.");

   params.film_size_mm = std::max(params.film_size_mm, 1.0f);
   params.focus_distance = std::max(params.focus_distance, 0.0f);
   params.f_stop = std::max(params.f_stop, 1.0f);

   return params;
}

SSAO_params show_ssao_imgui(SSAO_params params) noexcept
{
   ImGui::Checkbox("Enabled", &params.enabled);

   const ImVec2 pre_mode_cursor = ImGui::GetCursorPos();

   if (ImGui::RadioButton("Ambient", params.mode == SSAO_mode::ambient)) {
      params.mode = SSAO_mode::ambient;
   }

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
         "SSAO will affect ambient and vertex lighting only. This is more "
         "accurate. It produces a sublte effect in direct lighting and a more "
         "pronounced effect in shadows.");
   }

   ImGui::SameLine();

   if (ImGui::RadioButton("Global", params.mode == SSAO_mode::global)) {
      params.mode = SSAO_mode::global;
   }

   if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
         "SSAO will affect all lighting. This is less accurate "
         "and was the default before the Ambient mode was added.");
   }

   ImGui::SetCursorPos(pre_mode_cursor);

   ImGui::LabelText("Mode", "");

   ImGui::DragFloat("Radius", &params.radius, 0.1f, 0.1f, 2.0f);
   ImGui::DragFloat("Shadow Multiplier", &params.shadow_multiplier, 0.05f, 0.0f, 5.0f);
   ImGui::DragFloat("Shadow Power", &params.shadow_power, 0.05f, 0.0f, 5.0f);
   ImGui::DragFloat("Detail Shadow Strength", &params.detail_shadow_strength,
                    0.05f, 0.0f, 5.0f);
   ImGui::DragInt("Blur Amount", &params.blur_pass_count, 0.25f, 0, 6);
   ImGui::DragFloat("Sharpness", &params.sharpness, 0.01f, 0.0f, 1.0f);

   return params;
}

FFX_cas_params show_ffx_cas_imgui(FFX_cas_params params) noexcept
{
   ImGui::Checkbox("Enabled", &params.enabled);

   ImGui::DragFloat("Sharpness", &params.sharpness, 0.01f, 0.0f, 1.0f);

   params.sharpness = std::clamp(params.sharpness, 0.0f, 1.0f);

   return params;
}

void show_tonemapping_curve(std::function<float(float)> tonemapper) noexcept
{
   static float divisor = 256.0f;
   static int index_count = 1024;

   const auto plotter = [](void* data, int idx) {
      const auto& tonemapper = *static_cast<std::function<float(float)>*>(data);

      float v = idx / divisor;

      return tonemapper(v);
   };

   static int range = 0;

   ImGui::PlotLines("Tonemap Curve", plotter, &tonemapper, index_count);
   ImGui::SliderInt("Curve Preview Range", &range, 0, 4);

   ImGui::SameLine();

   switch (range) {
   case 0:
      divisor = 256.0f;
      index_count = 1024;
      ImGui::TextUnformatted("0.0 to 4.0");
      break;
   case 1:
      divisor = 256.0f;
      index_count = 2048;
      ImGui::TextUnformatted("0.0 to 8.0");
      break;
   case 2:
      divisor = 256.0f;
      index_count = 4096;
      ImGui::TextUnformatted("0.0 to 16.0");
      break;
   case 3:
      divisor = 128.0f;
      index_count = 4096;
      ImGui::TextUnformatted("0.0 to 32.0");
      break;
   case 4:
      divisor = 64.0f;
      index_count = 4096;
      ImGui::TextUnformatted("0.0 to 64.0");
      break;
   }
}
}
}
