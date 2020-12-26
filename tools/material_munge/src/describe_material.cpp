
#include "describe_material.hpp"
#include "compose_exception.hpp"
#include "glm_yaml_adapters.hpp"
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

constexpr auto max_textures = 64;

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

auto read_prop_op(const std::string_view op) -> Material_property_var_op
{
   // clang-format off
   if (op == "none"sv) return Material_property_var_op::none;
   if (op == "sqr"sv)  return Material_property_var_op::sqr;
   if (op == "sqrt"sv) return Material_property_var_op::sqrt;
   if (op == "exp"sv)  return Material_property_var_op::exp;
   if (op == "exp2"sv) return Material_property_var_op::exp2;
   if (op == "log"sv)  return Material_property_var_op::log;
   if (op == "log2"sv) return Material_property_var_op::log2;
   if (op == "rcp"sv)  return Material_property_var_op::rcp;
   if (op == "sign"sv) return Material_property_var_op::sign;
   // clang-format on

   throw std::runtime_error{"Invalid property op!"};
}

template<typename Type>
auto read_prop_value_scalar(const YAML::Node& value, const YAML::Node& desc,
                            const Material_property_var_op prop_op)
   -> Material_property::Value
{
   constexpr auto min = std::numeric_limits<Type>::min();
   constexpr auto max = std::numeric_limits<Type>::max();

   return {Material_var{value.as<Type>(), desc["Range"s][0].as<Type>(min),
                        desc["Range"s][1].as<Type>(max), prop_op}};
}

template<glm::length_t len, typename Type>
auto read_prop_value_vec(const YAML::Node& value, const YAML::Node& desc,
                         const Material_property_var_op prop_op)
   -> Material_property::Value
{
   using Vec = glm::vec<len, Type>;

   constexpr auto min = std::numeric_limits<Type>::min();
   constexpr auto max = std::numeric_limits<Type>::max();

   return {Material_var{value.as<Vec>(), Vec{desc["Range"s][0].as<Type>(min)},
                        Vec{desc["Range"s][1].as<Type>(max)}, prop_op}};
}

auto read_prop_value(const YAML::Node& value, const YAML::Node& desc)
   -> Material_property::Value
{
   const auto prop_op = read_prop_op(desc["Op"s].as<std::string>("none"s));

   if (const auto type = desc["Type"s].as<std::string>(); type == "float"sv) {
      return read_prop_value_scalar<float>(value, desc, prop_op);
   }
   else if (begins_with(type, "vec2"sv)) {
      return read_prop_value_vec<2, float>(value, desc, prop_op);
   }
   else if (begins_with(type, "vec3"sv)) {
      return read_prop_value_vec<3, float>(value, desc, prop_op);
   }
   else if (begins_with(type, "vec4"sv)) {
      return read_prop_value_vec<4, float>(value, desc, prop_op);
   }
   else if (type == "bool"sv) {
      return {Material_var{value.as<bool>(), false, true, prop_op}};
   }
   else if (type == "int"sv) {
      return read_prop_value_scalar<std::int32_t>(value, desc, prop_op);
   }
   else if (begins_with(type, "ivec2"sv)) {
      return read_prop_value_vec<2, glm::int32>(value, desc, prop_op);
   }
   else if (begins_with(type, "ivec3"sv)) {
      return read_prop_value_vec<3, glm::int32>(value, desc, prop_op);
   }
   else if (begins_with(type, "ivec4"sv)) {
      return read_prop_value_vec<4, glm::int32>(value, desc, prop_op);
   }
   else if (type == "uint"sv) {
      return read_prop_value_scalar<std::uint32_t>(value, desc, prop_op);
   }
   else if (begins_with(type, "uvec2"sv)) {
      return read_prop_value_vec<2, glm::uint>(value, desc, prop_op);
   }
   else if (begins_with(type, "uvec3"sv)) {
      return read_prop_value_vec<3, glm::uint>(value, desc, prop_op);
   }
   else if (begins_with(type, "uvec4"sv)) {
      return read_prop_value_vec<4, glm::uint>(value, desc, prop_op);
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

void read_texture_mapping(const std::string& texture_key, const YAML::Node& value,
                          const YAML::Node& texture_mappings,
                          Material_config& material_config)
{
   if (!texture_mappings[texture_key]) {
      throw compose_exception<std::runtime_error>("Undescribed material texture "sv,
                                                  std::quoted(texture_key),
                                                  " encountered."sv);
   }

   const auto texture_mapping_list = texture_mappings[texture_key];

   for (std::size_t i = 0; i < texture_mapping_list.size(); ++i) {
      const auto mapping = texture_mapping_list[i];

      const auto index = mapping["Slot"s].as<std::uint32_t>();

      if (index > max_textures) {
         throw compose_exception<std::runtime_error>("Invalid material texture description"sv,
                                                     std::quoted(texture_key),
                                                     " encountered. Texture index is too high!"sv);
      }

      auto& textures = [&]() -> std::vector<std::string>& {
         if (const auto stage = mapping["Stage"s].as<std::string>(); stage == "VS"sv)
            return material_config.vs_resources;
         else if (stage == "HS"sv)
            return material_config.hs_resources;
         else if (stage == "DS"sv)
            return material_config.ds_resources;
         else if (stage == "GS"sv)
            return material_config.gs_resources;
         else if (stage == "PS"sv)
            return material_config.ps_resources;

         throw compose_exception<std::runtime_error>("Invalid material texture description"sv,
                                                     std::quoted(texture_key),
                                                     " encountered. Texture stage binding is invalid!"sv);
      }();

      auto texture_name = value.as<std::string>();

      if (texture_name.empty()) return;

      if (index >= textures.size()) textures.resize(index + 1);

      textures.at(index) = value.as<std::string>();
   }
}

auto read_cb_stages(const YAML::Node& node) -> Material_cb_shader_stages
{
   Material_cb_shader_stages stages{};

   if (node["VS"s].as<bool>()) stages |= Material_cb_shader_stages::vs;
   if (node["HS"s].as<bool>()) stages |= Material_cb_shader_stages::hs;
   if (node["DS"s].as<bool>()) stages |= Material_cb_shader_stages::ds;
   if (node["GS"s].as<bool>()) stages |= Material_cb_shader_stages::gs;
   if (node["PS"s].as<bool>()) stages |= Material_cb_shader_stages::ps;

   return stages;
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
   error_check_properties(description, material);

   Material_config config{};

   config.name = name;
   config.rendertype = material["RenderType"s].as<std::string>();
   config.overridden_rendertype = rendertype_from_string(
      description["Overridden RenderType"s].as<std::string>());
   config.cb_name = description["Constant Buffer Name"s].as<std::string>();
   config.cb_shader_stages = read_cb_stages(description["Constant Buffer Stages"s]);
   config.fail_safe_texture_index =
      description["Failsafe Texture Index"s].as<std::uint32_t>();

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

   for (auto& texture : description["Default Textures"s]) {
      try {
         read_texture_mapping(texture.first.as<std::string>(), texture.second,
                              description["Texture Mappings"s], config);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material default texture "sv,
            std::quoted(texture.first.as<std::string>()), ": "sv, e.what());
      }
   }

   for (auto& texture : material["Textures"s]) {
      try {
         read_texture_mapping(texture.first.as<std::string>(), texture.second,
                              description["Texture Mappings"s], config);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material texture "sv,
            std::quoted(texture.first.as<std::string>()), ": "sv, e.what());
      }
   }

   if (description["Material Options"s])
      read_desc_material_options(description["Material Options"s], material_options);

   return config;
}
}
