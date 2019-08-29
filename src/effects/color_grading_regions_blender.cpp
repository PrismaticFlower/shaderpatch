
#include "color_grading_regions_blender.hpp"
#include "../logger.hpp"

#include <array>
#include <iomanip>

#include <gsl/gsl>

using namespace std::literals;

namespace sp::effects {

namespace {

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
}

void Color_grading_regions_blender::global_params(const Color_grading_params& params) noexcept
{
   _global_params = params;
}

auto Color_grading_regions_blender::global_params() const noexcept -> Color_grading_params
{
   return _global_params;
}

void Color_grading_regions_blender::regions(const Color_grading_regions& regions) noexcept
{
   _regions.clear();
   _region_params.clear();
   _region_names.clear();
   _region_params_names.clear();

   init_region_params(regions);
   init_regions(regions);
}

auto Color_grading_regions_blender::blend(const glm::vec3 camera_position) noexcept
   -> Color_grading_params
{
   contributions.clear();

   float global_weight = 1.0f;

   for (const auto& region : _regions) {
      const auto weight = region.weight(camera_position);

      if (weight <= 0.0f) continue;

      global_weight -= weight;

      contributions.push_back({weight, region.params});
   }

   if (global_weight > 0.0f) {
      contributions.push_back({global_weight, _global_params});
   }

   if (contributions.size() == 1) {
      return contributions.front().params;
   }

   const float total_weight =
      std::accumulate(contributions.cbegin(), contributions.cend(), 0.0f,
                      [](const auto v, const auto contrib) {
                         return v + contrib.weight;
                      });

   auto blended_params = apply_params_weight(contributions[0].params,
                                             contributions[0].weight / total_weight);

   for (auto i = 1; i < contributions.size(); ++i) {
      add_params(blended_params, contributions[i].params,
                 contributions[i].weight / total_weight);
   }

   blended_params.tonemapper = _global_params.tonemapper;

   return blended_params;
}

void Color_grading_regions_blender::show_imgui() noexcept {}

void Color_grading_regions_blender::init_region_params(const Color_grading_regions& regions) noexcept
{
   _region_params.reserve(regions.configs.size());
   _region_params_names.reserve(regions.configs.size());

   for (const auto& config : regions.configs) {
      const auto params = config.second["ColorGrading"s].as<Color_grading_params>(
         Color_grading_params{});

      _region_params.emplace_back(params);
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
   -> Color_grading_params&
{
   Expects(_region_params.size() == _region_params_names.size());

   for (auto i = 0; i < _region_params_names.size(); ++i) {
      if (_region_params_names[i] != config_name) continue;

      return _region_params[i];
   }

   log(Log_level::info, "Color Grading config "sv, std::quoted(config_name),
       " does not exist. Using default config.");

   _region_params_names.emplace_back(config_name);

   return _region_params.emplace_back(Color_grading_params{});
}

Color_grading_regions_blender::Region::Region(const Color_grading_region_desc& desc,
                                              const Color_grading_params& params) noexcept
   : params{params}
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