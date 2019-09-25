
#include "color_grading_regions_blender.hpp"
#include "../logger.hpp"
#include "file_dialogs.hpp"

#include <array>
#include <fstream>
#include <iomanip>

#include <imgui.h>

#include <gsl/gsl>

using namespace std::literals;

namespace sp::effects {

namespace {

constexpr auto max_unique_region_configs = std::numeric_limits<std::uint16_t>::max();

auto apply_params_weight(Color_grading_params params, const float weight) noexcept
{
   params.color_filter *= weight;
   params.saturation *= weight;
   params.exposure *= weight;
   params.brightness *= weight;
   params.contrast *= weight;

   params.filmic_toe_strength *= weight;
   params.filmic_toe_length *= weight;
   params.filmic_shoulder_strength *= weight;
   params.filmic_shoulder_length *= weight;
   params.filmic_shoulder_angle *= weight;

   params.filmic_heji_whitepoint *= weight;

   params.shadow_color *= weight;
   params.midtone_color *= weight;
   params.highlight_color *= weight;

   params.shadow_offset *= weight;
   params.midtone_offset *= weight;
   params.highlight_offset *= weight;

   params.hsv_hue_adjustment *= weight;
   params.hsv_saturation_adjustment *= weight;
   params.hsv_value_adjustment *= weight;

   params.channel_mix_red *= weight;
   params.channel_mix_green *= weight;
   params.channel_mix_blue *= weight;

   return params;
}

auto apply_params_weight(Bloom_params params, const float weight) noexcept
{
   params.threshold *= weight;
   params.intensity *= weight;
   params.tint *= weight;
   params.inner_scale *= weight;
   params.inner_tint *= weight;
   params.inner_mid_scale *= weight;
   params.inner_mid_tint *= weight;
   params.mid_scale *= weight;
   params.mid_tint *= weight;
   params.outer_mid_scale *= weight;
   params.outer_mid_tint *= weight;
   params.outer_scale *= weight;
   params.outer_tint *= weight;
   params.dirt_scale *= weight;
   params.dirt_tint *= weight;

   return params;
}

void add_params(Color_grading_params& dest, const Color_grading_params& src,
                const float src_weight) noexcept
{
   dest.color_filter += (src.color_filter * src_weight);
   dest.saturation += (src.saturation * src_weight);
   dest.exposure += (src.exposure * src_weight);
   dest.brightness += (src.brightness * src_weight);
   dest.contrast += (src.contrast * src_weight);

   dest.filmic_toe_strength += (src.filmic_toe_strength * src_weight);
   dest.filmic_toe_length += (src.filmic_toe_length * src_weight);
   dest.filmic_shoulder_strength += (src.filmic_shoulder_strength * src_weight);
   dest.filmic_shoulder_length += (src.filmic_shoulder_length * src_weight);
   dest.filmic_shoulder_angle += (src.filmic_shoulder_angle * src_weight);

   dest.filmic_heji_whitepoint += (src.filmic_heji_whitepoint * src_weight);

   dest.shadow_color += (src.shadow_color * src_weight);
   dest.midtone_color += (src.midtone_color * src_weight);
   dest.highlight_color += (src.highlight_color * src_weight);

   dest.shadow_offset += (src.shadow_offset * src_weight);
   dest.midtone_offset += (src.midtone_offset * src_weight);
   dest.highlight_offset += (src.highlight_offset * src_weight);

   dest.hsv_hue_adjustment += (src.hsv_hue_adjustment * src_weight);
   dest.hsv_saturation_adjustment += (src.hsv_saturation_adjustment * src_weight);
   dest.hsv_value_adjustment += (src.hsv_value_adjustment * src_weight);

   dest.channel_mix_red += (src.channel_mix_red * src_weight);
   dest.channel_mix_green += (src.channel_mix_green * src_weight);
   dest.channel_mix_blue += (src.channel_mix_blue * src_weight);
}

void add_params(Bloom_params& dest, const Bloom_params& src, const float src_weight) noexcept
{
   dest.threshold += (src.threshold * src_weight);
   dest.intensity += (src.intensity * src_weight);
   dest.tint += (src.tint * src_weight);
   dest.inner_scale += (src.inner_scale * src_weight);
   dest.inner_tint += (src.inner_tint * src_weight);
   dest.inner_mid_scale += (src.inner_mid_scale * src_weight);
   dest.inner_mid_tint += (src.inner_mid_tint * src_weight);
   dest.mid_scale += (src.mid_scale * src_weight);
   dest.mid_tint += (src.mid_tint * src_weight);
   dest.outer_mid_scale += (src.outer_mid_scale * src_weight);
   dest.outer_mid_tint += (src.outer_mid_tint * src_weight);
   dest.outer_scale += (src.outer_scale * src_weight);
   dest.outer_tint += (src.outer_tint * src_weight);
   dest.dirt_scale += (src.dirt_scale * src_weight);
   dest.dirt_tint += (src.dirt_tint * src_weight);
}

void save_configs(const std::filesystem::path& path,
                  const std::vector<Color_grading_params>& params,
                  const std::vector<std::optional<Bloom_params>>& bloom_params,
                  const std::vector<std::string>& names) noexcept
{
   Expects(params.size() == names.size());

   if (!std::filesystem::is_directory(path)) {
      log(Log_level::error,
          "Attempt to save color grading region configs to nonexistant directory!"sv);

      return;
   }

   for (auto i = 0; i < params.size(); ++i) {
      const auto config_path = path / names[i] += L".clrfx"sv;

      YAML::Node yaml;

      yaml["ColorGrading"s] = params[i];
      if (bloom_params[i]) yaml["Bloom"s] = *bloom_params[i];

      std::ofstream file{config_path};
      YAML::Emitter out{file};

      out.SetBoolFormat(YAML::YesNoBool);
      out.SetSeqFormat(YAML::Flow);
      out.SetLocalIndent(3);

      out << yaml;
   }
}

}

void Color_grading_regions_blender::global_cg_params(const Color_grading_params& params) noexcept
{
   _global_cg_params = params;
}

auto Color_grading_regions_blender::global_bloom_params() const noexcept -> Bloom_params
{
   return _global_bloom_params;
}

void Color_grading_regions_blender::global_bloom_params(const Bloom_params& params) noexcept
{
   _global_bloom_params = params;
}

auto Color_grading_regions_blender::global_cg_params() const noexcept -> Color_grading_params
{
   return _global_cg_params;
}

void Color_grading_regions_blender::regions(const Color_grading_regions& regions) noexcept
{
   _regions.clear();
   _region_cg_params.clear();
   _region_bloom_params.clear();
   _region_names.clear();
   _region_params_names.clear();
   _imgui_editor_state.clear();

   init_region_params(regions);
   init_regions(regions);

   _contributions.reserve(_regions.size() + 1);
}

auto Color_grading_regions_blender::blend(const glm::vec3 camera_position) noexcept
   -> std::pair<Color_grading_params, Bloom_params>
{
   _contributions.clear();

   float global_weight = 1.0f;

   for (const auto& region : _regions) {
      const auto weight = region.weight(camera_position);

      if (weight <= 0.0f) continue;

      global_weight -= weight;

      _contributions.push_back({weight, _region_cg_params[region.params_index],
                                _region_bloom_params[region.params_index]
                                   ? *_region_bloom_params[region.params_index]
                                   : _global_bloom_params});
   }

   if (global_weight > 0.0f) {
      _contributions.push_back({global_weight, _global_cg_params, _global_bloom_params});
   }

   if (_contributions.size() == 1) {
      auto cg_params = _contributions.front().cg_params;
      auto bloom_params = _contributions.front().bloom_params;

      cg_params.tonemapper = _global_cg_params.tonemapper;

      return {std::move(cg_params), std::move(bloom_params)};
   }

   const float total_weight =
      std::accumulate(_contributions.cbegin(), _contributions.cend(), 0.0f,
                      [](const auto v, const auto contrib) {
                         return v + contrib.weight;
                      });

   auto blended_cg_params =
      apply_params_weight(_contributions[0].cg_params,
                          _contributions[0].weight / total_weight);
   auto blended_bloom_params =
      apply_params_weight(_contributions[0].bloom_params,
                          _contributions[0].weight / total_weight);

   for (auto i = 1; i < _contributions.size(); ++i) {
      const float weight = _contributions[i].weight / total_weight;

      add_params(blended_cg_params, _contributions[i].cg_params, weight);
      add_params(blended_bloom_params, _contributions[i].bloom_params, weight);
   }

   blended_cg_params.tonemapper = _global_cg_params.tonemapper;

   return {std::move(blended_cg_params), std::move(blended_bloom_params)};
}

void Color_grading_regions_blender::show_imgui(
   const HWND game_window,
   Small_function<Color_grading_params(Color_grading_params) noexcept> show_cg_params_imgui,
   Small_function<Bloom_params(Bloom_params) noexcept> show_bloom_params_imgui) noexcept
{
   Expects(_region_cg_params.size() == _region_params_names.size());

   _imgui_editor_state.resize(_region_cg_params.size());

   if (ImGui::BeginTabBar("Color Grading Regions", ImGuiTabBarFlags_AutoSelectNewTabs)) {
      if (ImGui::BeginTabItem("Configs")) {
         if (ImGui::Button("Save Configs")) {
            if (auto path = win32::folder_dialog(game_window); path) {
               save_configs(*path, _region_cg_params, _region_bloom_params,
                            _region_params_names);
            }
         }

         if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Save out configs the for 'spfx_munge'.");
         }

         ImGui::Separator();

         ImGui::Text("Select a config to open an editor for it.");

         ImGui::Separator();

         for (auto i = 0; i < _region_params_names.size(); ++i) {
            ImGui::Selectable(_region_params_names[i].data(), _imgui_editor_state[i]);
         }

         ImGui::EndTabItem();
      }

      for (auto i = 0; i < _region_params_names.size(); ++i) {
         if (!_imgui_editor_state[i]) continue;

         if (ImGui::BeginTabItem(_region_params_names[i].data(),
                                 _imgui_editor_state[i])) {
            if (ImGui::BeginTabBar("Config Params", ImGuiTabBarFlags_None)) {
               if (ImGui::BeginTabItem("Color Grading")) {
                  _region_cg_params[i] = show_cg_params_imgui(_region_cg_params[i]);

                  ImGui::EndTabItem();
               }

               if ((_global_cg_params.tonemapper == Tonemapper::filmic ||
                    _global_cg_params.tonemapper == Tonemapper::filmic_heji2015) &&
                   ImGui::BeginTabItem("Tonemapping")) {
                  if (_global_cg_params.tonemapper == Tonemapper::filmic) {
                     ImGui::DragFloat("Toe Strength",
                                      &_region_cg_params[i].filmic_toe_strength,
                                      0.01f, 0.0f, 1.0f);
                     ImGui::DragFloat("Toe Length",
                                      &_region_cg_params[i].filmic_toe_length,
                                      0.01f, 0.0f, 1.0f);
                     ImGui::DragFloat("Shoulder Strength",
                                      &_region_cg_params[i].filmic_shoulder_strength,
                                      0.01f, 0.0f, 100.0f);
                     ImGui::DragFloat("Shoulder Length",
                                      &_region_cg_params[i].filmic_shoulder_length,
                                      0.01f, 0.0f, 1.0f);
                     ImGui::DragFloat("Shoulder Angle",
                                      &_region_cg_params[i].filmic_shoulder_angle,
                                      0.01f, 0.0f, 1.0f);
                  }

                  if (_global_cg_params.tonemapper == Tonemapper::filmic_heji2015) {
                     ImGui::DragFloat("Whitepoint",
                                      &_region_cg_params[i].filmic_heji_whitepoint,
                                      0.01f);
                  }

                  ImGui::EndTabItem();
               }

               bool bloom_params = _region_bloom_params[i].has_value();

               if (bloom_params && ImGui::BeginTabItem("Bloom", &bloom_params)) {
                  _region_bloom_params[i] =
                     show_bloom_params_imgui(*_region_bloom_params[i]);

                  ImGui::EndTabItem();
               }

               if (!bloom_params) {
                  _region_bloom_params[i] = std::nullopt;

                  ImGui::Separator();

                  if (ImGui::Button("Enable Bloom Control")) {
                     _region_bloom_params[i].emplace();
                  }
               }

               ImGui::EndTabBar();
            }

            ImGui::EndTabItem();
         }
      }

      ImGui::EndTabBar();
   }
}

void Color_grading_regions_blender::init_region_params(const Color_grading_regions& regions) noexcept
{
   if (regions.configs.size() > max_unique_region_configs) {
      log_and_terminate(
         "Too many color grading region configs. Max supported is 65535.");
   }

   _region_cg_params.reserve(regions.configs.size());
   _region_bloom_params.reserve(regions.configs.size());
   _region_params_names.reserve(regions.configs.size());

   for (const auto& config : regions.configs) {
      _region_cg_params.emplace_back(
         config.second["ColorGrading"s].as<Color_grading_params>(Color_grading_params{}));
      _region_bloom_params.push_back(
         config.second["Bloom"s]
            ? std::optional{config.second["Bloom"s].as<Bloom_params>()}
            : std::nullopt);
      _region_params_names.emplace_back(config.first);
   }
}

void Color_grading_regions_blender::init_regions(const Color_grading_regions& regions) noexcept
{
   _regions.reserve(regions.regions.size());
   _region_names.reserve(regions.regions.size());

   for (const auto& region : regions.regions) {
      const auto inv_fade_length =
         region.fade_length <= 0.0f ? 10000.0f : (1.0f / region.fade_length);

      _regions.emplace_back(region, get_region_params(region.config_name));
      _region_names.emplace_back(region.name);
   }
}

auto Color_grading_regions_blender::get_region_params(const std::string_view config_name) noexcept
   -> std::size_t
{
   Expects(_region_cg_params.size() == _region_params_names.size());

   for (std::size_t i = 0; i < _region_params_names.size(); ++i) {
      if (_region_params_names[i] != config_name) continue;

      return i;
   }

   log(Log_level::info, "Color Grading config "sv, std::quoted(config_name),
       " does not exist. Using default config.");

   _region_params_names.emplace_back(config_name);
   _region_cg_params.emplace_back();
   _region_bloom_params.emplace_back();

   if (_region_cg_params.size() > max_unique_region_configs) {
      log_and_terminate("Too many unique color grading region configs. Max "
                        "supported is 65535.");
   }

   return _region_cg_params.size() - 1;
}

Color_grading_regions_blender::Region::Region(const Color_grading_region_desc& desc,
                                              const std::size_t params_index) noexcept
   : params_index{params_index}
{
   const auto inv_fade_length =
      desc.fade_length <= 0.0f ? 1e6f : (1.0f / desc.fade_length);

   if (desc.shape == Color_grading_region_shape::box) {
      primitive = Box{.rotation = glm::inverse(desc.rotation),
                      .centre = desc.position,
                      .length = desc.size * 2.0f,
                      .inv_fade_length = inv_fade_length};
   }
   else if (desc.shape == Color_grading_region_shape::sphere) {
      primitive = Sphere{.centre = desc.position,
                         .radius = glm::length(desc.size),
                         .inv_fade_length = inv_fade_length};
   }
   else if (desc.shape == Color_grading_region_shape::cylinder) {
      primitive =
         Cylinder{.rotation = glm::inverse(desc.rotation),
                  .centre = desc.position,
                  .radius = glm::length(glm::vec2{desc.size.x, desc.size.z}),
                  .length = desc.size.y,
                  .inv_fade_length = inv_fade_length};
   }
}

}