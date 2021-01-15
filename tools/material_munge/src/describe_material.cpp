
#include "describe_material.hpp"
#include "compose_exception.hpp"
#include "glm_yaml_adapters.hpp"
#include "material_rendertype_property_mappings.hpp"
#include "string_utilities.hpp"
#include "utility.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <string>
#include <string_view>

#include <gsl/gsl>

namespace sp {

using namespace std::literals;

namespace {

void convert_old_material(YAML::Node material)
{
   auto rendertype = material["RenderType"s].as<std::string>();
   auto root_type = split_string_on(rendertype, "."sv)[0];

   if (root_type == "normal_ext-tessellated"sv) root_type = "normal_ext"sv;

   material["Type"s] = std::string{root_type};
   material.remove("RenderType"s);

   if (!rendertype_property_mappings.contains(root_type)) return;

   auto properties = material["Material"s];

   for (const auto& [str, prop] : rendertype_property_mappings.at(root_type)) {
      if (!contains(rendertype, str)) continue;

      std::visit([&](const auto& value) { properties[prop.name] = value.value; },
                 prop.value);
   }
}

void error_check_properties(const YAML::Node& description, const YAML::Node& material)
{
   for (const auto& prop : material["Material"s]) {
      auto desc = description["Properties"s][prop.first.as<std::string>()];

      if (!desc) {
         throw compose_exception<std::runtime_error>("Undescribed material property "sv,
                                                     std::quoted(
                                                        prop.first.as<std::string>()),
                                                     " encountered."sv);
      }
   }
}

template<typename Type>
auto read_prop_value_scalar(const YAML::Node& value, const YAML::Node& desc)
   -> Material_property::Value
{
   constexpr auto min = std::numeric_limits<Type>::min();
   constexpr auto max = std::numeric_limits<Type>::max();

   return {Material_var{value.as<Type>(), desc["Range"s][0].as<Type>(min),
                        desc["Range"s][1].as<Type>(max)}};
}

template<glm::length_t len, typename Type>
auto read_prop_value_vec(const YAML::Node& value, const YAML::Node& desc)
   -> Material_property::Value
{
   using Vec = glm::vec<len, Type>;

   constexpr auto min = std::numeric_limits<Type>::min();
   constexpr auto max = std::numeric_limits<Type>::max();

   return {Material_var{value.as<Vec>(), Vec{desc["Range"s][0].as<Type>(min)},
                        Vec{desc["Range"s][1].as<Type>(max)}}};
}

auto read_prop_value(const YAML::Node& value, const YAML::Node& desc)
   -> Material_property::Value
{
   if (const auto type = desc["Type"s].as<std::string>(); type == "float"sv) {
      return read_prop_value_scalar<float>(value, desc);
   }
   else if (begins_with(type, "vec2"sv)) {
      return read_prop_value_vec<2, float>(value, desc);
   }
   else if (begins_with(type, "vec3"sv)) {
      return read_prop_value_vec<3, float>(value, desc);
   }
   else if (begins_with(type, "vec4"sv)) {
      return read_prop_value_vec<4, float>(value, desc);
   }
   else if (type == "bool"sv) {
      return {Material_var{value.as<bool>(), false, true}};
   }
   else if (type == "int"sv) {
      return read_prop_value_scalar<std::int32_t>(value, desc);
   }
   else if (begins_with(type, "ivec2"sv)) {
      return read_prop_value_vec<2, glm::int32>(value, desc);
   }
   else if (begins_with(type, "ivec3"sv)) {
      return read_prop_value_vec<3, glm::int32>(value, desc);
   }
   else if (begins_with(type, "ivec4"sv)) {
      return read_prop_value_vec<4, glm::int32>(value, desc);
   }
   else if (type == "uint"sv) {
      return read_prop_value_scalar<std::uint32_t>(value, desc);
   }
   else if (begins_with(type, "uvec2"sv)) {
      return read_prop_value_vec<2, glm::uint>(value, desc);
   }
   else if (begins_with(type, "uvec3"sv)) {
      return read_prop_value_vec<3, glm::uint>(value, desc);
   }
   else if (begins_with(type, "uvec4"sv)) {
      return read_prop_value_vec<4, glm::uint>(value, desc);
   }
   else {
      throw compose_exception<std::runtime_error>("Unknown material property type "sv,
                                                  std::quoted(type), "."sv);
   }
}

auto read_prop(std::string prop_key, const YAML::Node& desc,
               const YAML::Node& material_props) -> Material_property
{
   return {std::move(prop_key),
           read_prop_value(material_props[prop_key] ? material_props[prop_key]
                                                    : desc["Default"s],
                           desc)};
}

auto read_resource_property(const std::string& key, const std::string& value,
                            const YAML::Node& resource_properties)
   -> std::pair<std::string, std::string>
{
   if (!resource_properties[key]) {
      throw compose_exception<std::runtime_error>("Undescribed material resource "sv,
                                                  std::quoted(key), " encountered."sv);
   }

   return {key, value};
}

auto read_desc_material_options(const YAML::Node& node, Material_options& material_options)
{
   for (const auto& opt : node) {
      if (const auto key = opt.first.as<std::string>(); key == "Transparent"s)
         material_options.forced_transparent_value = opt.second.as<bool>();
      else if (key == "Hard Edged"s)
         material_options.forced_hard_edged_value = opt.second.as<bool>();
      else if (key == "Double Sided"s)
         material_options.forced_double_sided_value = opt.second.as<bool>();
      else if (key == "Unlit"s)
         material_options.forced_unlit_value = opt.second.as<bool>();
      else if (key == "Compressed"s)
         material_options.compressed = opt.second.as<bool>();
      else if (key == "Generate Tangents"s)
         material_options.generate_tangents = opt.second.as<bool>();
      else
         throw compose_exception<std::runtime_error>("Invalid material option in rendertype description "sv,
                                                     std::quoted(key), "."sv);
   }
}

}

auto describe_material(const std::string_view name,
                       const YAML::Node& description, const YAML::Node& material,
                       Material_options& material_options) -> Material_config
{
   if (material["RenderType"s]) convert_old_material(material);

   error_check_properties(description, material);

   Material_config config{};

   config.name = name;
   config.type = material["Type"s].as<std::string>();
   config.overridden_rendertype = rendertype_from_string(
      description["Overridden RenderType"s].as<std::string>());

   for (auto& prop : description["Properties"s]) {
      try {
         config.properties.emplace_back(read_prop(prop.first.as<std::string>(),
                                                  prop.second, material["Material"s]));
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material property "sv,
            std::quoted(prop.first.as<std::string>()), ": "sv, e.what());
      }
   }

   for (auto& texture : material["Textures"s]) {
      try {
         auto texture_property = texture.first.as<std::string>();
         auto texture_name = texture.second.as<std::string>();

         if (texture_name.empty()) continue;

         config.resources.emplace(
            read_resource_property(texture_property, texture_name,
                                   description["Resource Properties"s]));
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material texture "sv,
            std::quoted(texture.first.as<std::string>()), ": "sv, e.what());
      }
   }

   if (description["Material Options"s]) {
      read_desc_material_options(description["Material Options"s], material_options);
   }

   return config;
}
}
