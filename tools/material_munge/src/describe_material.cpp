
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
   if (op == "sign"sv)  return Material_property_var_op::sign;
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

auto read_topology(const YAML::Node& node) -> D3D_PRIMITIVE_TOPOLOGY
{
   if (!node) return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

   const auto topology = node.as<std::string>();

   // clang-format off
   if (topology == "pointlist"sv) return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
   if (topology == "linelist"sv) return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
   if (topology == "linestrip"sv) return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
   if (topology == "trianglelist"sv) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   if (topology == "trianglestrip"sv) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
   if (topology == "linelist_adj"sv) return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
   if (topology == "linestrip_adj"sv) return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
   if (topology == "trianglelist_adj"sv) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
   if (topology == "trianglestrip_adj"sv) return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
   if (topology == "1_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
   if (topology == "2_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
   if (topology == "3_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
   if (topology == "4_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
   if (topology == "5_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
   if (topology == "6_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
   if (topology == "7_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
   if (topology == "8_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
   if (topology == "9_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
   if (topology == "10_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
   if (topology == "11_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
   if (topology == "12_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
   if (topology == "13_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
   if (topology == "14_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
   if (topology == "15_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
   if (topology == "16_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
   if (topology == "17_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
   if (topology == "18_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
   if (topology == "19_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
   if (topology == "20_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
   if (topology == "21_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
   if (topology == "22_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
   if (topology == "23_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
   if (topology == "24_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
   if (topology == "25_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
   if (topology == "26_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
   if (topology == "27_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
   if (topology == "28_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
   if (topology == "29_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
   if (topology == "30_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
   if (topology == "31_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
   if (topology == "32_control_point_patchlist"sv) return D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
   // clang-format on

   throw compose_exception<std::runtime_error>("Invalid primitive topology in rendertype description "sv,
                                               std::quoted(topology), "."sv);
}

auto read_desc_material_options(const YAML::Node& node, Material_options& material_options)
{
   for (const auto& opt : node) {
      if (const auto key = opt.first.as<std::string>(); key == "Transparent"s)
         material_options.transparent = opt.second.as<bool>();
      else if (key == "Hard Edged"s)
         material_options.hard_edged = opt.second.as<bool>();
      else if (key == "Double Sided"s)
         material_options.double_sided = opt.second.as<bool>();
      else if (key == "Statically Lit"s)
         material_options.statically_lit = opt.second.as<bool>();
      else if (key == "Unlit"s)
         material_options.unlit = opt.second.as<bool>();
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
   config.tessellation = description["Tessellation"s].as<bool>(false);
   config.tessellation_primitive_topology =
      read_topology(description["Tessellation Primitive Topology"s]);

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
