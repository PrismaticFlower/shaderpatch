
#include "describe_material.hpp"
#include "compose_exception.hpp"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <string>
#include <string_view>

#include <gsl/gsl>

namespace sp {

using namespace std::literals;

namespace {

constexpr auto max_textures = 64;

constexpr auto read_constant_offset(std::string_view string)
{
   const auto allowed_numeric_part = "0123456789"sv;
   const auto allowed_index_part = "xyzw"sv;

   const auto npos = std::string_view::npos;

   if (string.length() != 3 || string[1] != '.' ||
       allowed_numeric_part.find(string[0]) == npos ||
       allowed_index_part.find(string[2]) == npos) {
      throw compose_exception<std::runtime_error>(
         "Bad constant offset described for material property "sv);
   }

   gsl::index offset = (string[0] - '0') * 4;

   if (string[2] == 'x') offset += 0;
   if (string[2] == 'y') offset += 1;
   if (string[2] == 'z') offset += 2;
   if (string[2] == 'w') offset += 3;

   return offset;
}

template<typename Value>
auto set_constant(Aligned_vector<std::byte, 16>& cb,
                  const std::size_t byte_offset, const Value value)
{
   static_assert(std::is_standard_layout_v<Value>);

   const auto needed_size = byte_offset + sizeof(Value);

   if (cb.size() < needed_size) cb.resize(needed_size);

   std::memcpy(cb.data() + byte_offset, &value, sizeof(Value));
}

auto apply_prop_op(const float val, std::string_view op) -> float
{
   if (op == "sqr"sv) {
      return val * val;
   }
   else if (op == "sqrt"sv) {
      return std::sqrt(val);
   }
   else if (op == "exp"sv) {
      return std::exp(val);
   }
   else if (op == "exp2"sv) {
      return std::exp2(val);
   }
   else if (op == "log"sv) {
      return std::log(val);
   }
   else if (op == "log2"sv) {
      return std::log2(val);
   }

   throw std::runtime_error{"Invalid property op!"};
}

auto read_prop(const std::string& prop_key, const YAML::Node value,
               const YAML::Node property_mappings, Aligned_vector<std::byte, 16>& cb)
{
   if (!property_mappings[prop_key]) {
      throw compose_exception<std::runtime_error>("Undescribed material property "sv,
                                                  std::quoted(prop_key),
                                                  " encountered."sv);
   }

   auto desc = property_mappings[prop_key];

   const auto type = desc["Type"s].as<std::string>();
   const std::array<float, 2> range{desc["Range"s][0].as<float>(),
                                    desc["Range"s][1].as<float>()};
   const auto offset = read_constant_offset(desc["Constant"s].as<std::string>());

   if (type == "scaler"sv) {
      const auto byte_offset = offset * sizeof(float);
      const auto clamped_value = std::clamp(value.as<float>(), range[0], range[1]);
      const auto final_value =
         desc["Op"s] ? apply_prop_op(clamped_value, desc["Op"s].as<std::string>())
                     : clamped_value;

      set_constant(cb, byte_offset, final_value);
   }
   else {
      auto count = 0;

      // clang-format off
      if (type == "vec2"sv) count = 2;
      else if (type == "vec3"sv) count = 3;
      else if (type == "vec4"sv) count = 4;
      else {
         // clang-format on

         throw compose_exception<std::runtime_error>("Invalid material property type "sv,
                                                     std::quoted(prop_key),
                                                     " encountered in description."sv);
      }

      for (auto i = 0; i < count; ++i) {
         const auto byte_offset = offset * sizeof(float) + (i * sizeof(float));
         const auto clamped_value =
            std::clamp(value[i].as<float>(), range[0], range[1]);
         const auto final_value =
            desc["Op"s]
               ? apply_prop_op(clamped_value, desc["Op"s].as<std::string>())
               : clamped_value;

         set_constant(cb, byte_offset, final_value);
      }
   }
}

void read_texture_mapping(const std::string& texture_key, const YAML::Node value,
                          const YAML::Node texture_mappings, Material_info& material_info)
{
   if (!texture_mappings[texture_key]) {
      throw compose_exception<std::runtime_error>("Undescribed material texture "sv,
                                                  std::quoted(texture_key),
                                                  " encountered."sv);
   }

   auto texture = texture_mappings[texture_key];

   auto index = texture["Slot"s].as<int>();

   if (index > max_textures) {
      throw compose_exception<std::runtime_error>("Invalid material texture description"sv,
                                                  std::quoted(texture_key),
                                                  " encountered. Texture index is too high!"sv);
   }

   auto texture_name = value.as<std::string>();

   if (texture_name.empty()) return;

   if (index >= material_info.textures.size())
      material_info.textures.resize(index + 1);

   material_info.textures.at(index) = value.as<std::string>();
}
}

auto describe_material(std::string_view name, const YAML::Node description,
                       const YAML::Node material) -> Material_info
{
   Material_info info{};

   info.name = name;
   info.rendertype = material["RenderType"s].as<std::string>();
   info.overridden_rendertype = rendertype_from_string(
      description["Overridden RenderType"s].as<std::string>());

   for (auto prop : material["Material"s]) {
      try {
         read_prop(prop.first.as<std::string>(), prop.second,
                   description["Property Mappings"s], info.constant_buffer);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material property "sv,
            std::quoted(prop.first.as<std::string>()), ": "sv, e.what());
      }
   }

   for (auto texture : material["Textures"s]) {
      try {
         read_texture_mapping(texture.first.as<std::string>(), texture.second,
                              description["Texture Mappings"s], info);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing material texture "sv,
            std::quoted(texture.first.as<std::string>()), ": "sv, e.what());
      }
   }

   return info;
}
}
