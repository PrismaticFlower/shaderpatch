
#include "munge_colorgrading_regions.hpp"
#include "color_grading_regions_io.hpp"
#include "config_file.hpp"
#include "string_utilities.hpp"

#include <algorithm>
#include <charconv>

#include <gsl/gsl>

using namespace std::literals;

namespace sp {

constexpr auto region_shape_box = 0;
constexpr auto region_shape_sphere = 1;
constexpr auto region_shape_cylinder = 2;

namespace {
auto parse_region_type_string(const std::string_view string)
   -> std::pair<std::string_view, float>
{
   Expects(!string.empty());

   const auto [name, props] = split_string_on(string, " "sv);
   const auto [config, fadelength] = split_string_on(props, " "sv);
   const auto [config_key, config_value] = split_string_on(config, "="sv);
   const auto [fade_key, fade_value] = split_string_on(fadelength, "="sv);

   if (config_value.empty() || fade_value.empty()) {
      throw std::runtime_error{
         "bad properties in region, Config and FadeLength must not be empty"};
   }

   float fadelength_value = 0.0f;

   if (const auto result =
          std::from_chars(&fade_value[0], &fade_value[0] + fade_value.size(),
                          fadelength_value);
       result.ec != std::errc{}) {
      throw std::runtime_error{"bad fade length in region"};
   }

   return {config_value, fadelength_value};
}

auto get_region_shape(const int shape) -> Color_grading_region_shape
{
   switch (shape) {
   case region_shape_box:
      return Color_grading_region_shape::box;
   case region_shape_sphere:
      return Color_grading_region_shape::sphere;
   case region_shape_cylinder:
      return Color_grading_region_shape::cylinder;
   }

   throw std::runtime_error{"Invalid region shape!"};
}

auto read_region_node(const cfg::Node& node) -> Color_grading_region_desc
{
   const auto type_string = node.get_value<std::string>(0);

   if (!begins_with(type_string, "colorgrading"sv)) {
      throw std::runtime_error{
         "Region in colorgrading layer does not have \"colorgrading\" type!"};
   }

   Color_grading_region_desc desc;

   desc.shape = get_region_shape(node.get_value<int>(1));

   std::tie(desc.config_name, desc.fade_length) =
      parse_region_type_string(type_string);

   desc.position = [&] {
      const auto it = find(node, "Position"sv);

      if (it == node.cend()) {
         throw std::runtime_error{"invalid region file"};
      }

      return glm::vec3{it->second.get_value<float>(0),
                       it->second.get_value<float>(1),
                       it->second.get_value<float>(2) * -1.0f};
   }();

   desc.rotation = [&] {
      const auto it = find(node, "Rotation"sv);

      if (it == node.cend()) {
         throw std::runtime_error{"invalid region file"};
      }

      return glm::quat{it->second.get_value<float>(0),
                       it->second.get_value<float>(1),
                       it->second.get_value<float>(2),
                       it->second.get_value<float>(3)};
   }();

   desc.size = [&] {
      const auto it = find(node, "Size"sv);

      if (it == node.cend()) {
         throw std::runtime_error{"invalid region file"};
      }

      return glm::vec3{it->second.get_value<float>(0),
                       it->second.get_value<float>(1),
                       it->second.get_value<float>(2)};
   }();

   desc.name = [&] {
      const auto it = find(node, "Name"sv);

      if (it == node.cend()) {
         throw std::runtime_error{"invalid region file"};
      }

      return it->second.get_value<std::string>(0);
   }();

   return desc;
}

auto load_colorgrading_regions(const std::filesystem::path& regions_path)
   -> std::vector<Color_grading_region_desc>
{
   const auto regions = cfg::load_file(regions_path);

   const auto count = [&] {
      const auto it = find(regions, "RegionCount"sv);

      if (it == regions.cend()) {
         throw std::runtime_error{"invalid region file"};
      }

      return it->second.get_value<std::uint32_t>(0);
   }();

   std::vector<Color_grading_region_desc> result;
   result.reserve(count);

   for (const auto& node : regions) {
      if (node.first != "Region"sv) continue;

      result.emplace_back(read_region_node(node.second));
   }

   return result;
}

auto load_colorgrading_configs(const std::filesystem::path& config_search_path,
                               const std::vector<Color_grading_region_desc>& descs)
   -> std::unordered_map<std::string, YAML::Node>
{
   std::unordered_map<std::string, YAML::Node> configs;
   configs.reserve(descs.size());

   for (const auto& entry :
        std::filesystem::recursive_directory_iterator{config_search_path}) {
      if (!entry.is_regular_file()) continue;
      if (entry.path().extension() != L".clrfx"sv) continue;

      auto name = entry.path().stem().string();

      if (std::find_if(descs.cbegin(), descs.cend(), [&name](const auto& desc) {
             return desc.config_name == name;
          }) == descs.cend()) {
         continue;
      }

      configs.emplace(std::move(name), YAML::LoadFile(entry.path().string()));
   }

   return configs;
}

}

void munge_colorgrading_regions(const std::filesystem::path& regions_path,
                                const std::filesystem::path& config_search_path,
                                const std::filesystem::path& output_path) noexcept
{
   Color_grading_regions colorgrading;

   colorgrading.regions = load_colorgrading_regions(regions_path);
   colorgrading.configs =
      load_colorgrading_configs(config_search_path, colorgrading.regions);

   auto output_filename = regions_path.filename();
   output_filename.replace_extension(L".color_regions"sv);

   write_colorgrading_regions(output_path / output_filename, colorgrading);
}
}