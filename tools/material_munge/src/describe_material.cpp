
#include "describe_material.hpp"
#include "compose_exception.hpp"
#include "string_utilities.hpp"
#include "utility.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
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

auto get_vec_size(const std::string_view string) -> std::size_t
{
   const auto count = split_string_on(string, "vec"sv)[1];

   if (count.empty()) {
      throw compose_exception<std::runtime_error>(
         "Invalid material property type encountered in description."sv);
   }

   std::size_t size{};

   std::from_chars(count.data(), count.data() + count.size(), size);

   return size;
}

template<typename Value>
auto set_constant(Aligned_vector<std::byte, 16>& cb,
                  const std::size_t cb_offset, const Value value)
{
   static_assert(std::is_standard_layout_v<Value>);
   static_assert(sizeof(Value) == 4);

   const auto byte_offset = cb_offset * sizeof(Value);
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
   else if (op == "rcp"sv) {
      return 1.0f / val;
   }

   throw std::runtime_error{"Invalid property op!"};
}

void process_float_prop(const YAML::Node& value, const YAML::Node& desc,
                        const std::size_t count, Aligned_vector<std::byte, 16>& cb)
{
   const std::array<float, 2> range{desc["Range"s][0].as<float>(),
                                    desc["Range"s][1].as<float>()};
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());

   for (auto i = 0; i < count; ++i) {
      const auto clamped_value =
         std::clamp(value[i].as<float>(), range[0], range[1]);

      const auto final_value =
         desc["Op"s] ? apply_prop_op(clamped_value, desc["Op"s].as<std::string>())
                     : clamped_value;

      set_constant(cb, cb_offset + i, final_value);
   }
}

void process_float_prop(const YAML::Node& value, const YAML::Node& desc,
                        Aligned_vector<std::byte, 16>& cb)
{
   const std::array<float, 2> range{desc["Range"s][0].as<float>(),
                                    desc["Range"s][1].as<float>()};
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());

   const auto clamped_value = std::clamp(value.as<float>(), range[0], range[1]);
   const auto final_value =
      desc["Op"s] ? apply_prop_op(clamped_value, desc["Op"s].as<std::string>())
                  : clamped_value;

   set_constant(cb, cb_offset, final_value);
}

void process_bool_prop(const YAML::Node& value, const YAML::Node& desc,
                       const std::size_t count, Aligned_vector<std::byte, 16>& cb)
{
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());

   for (auto i = 0; i < count; ++i) {
      const auto uint_value = static_cast<std::uint32_t>(value[i].as<bool>());

      set_constant(cb, cb_offset + i, uint_value);
   }
}

void process_bool_prop(const YAML::Node& value, const YAML::Node& desc,
                       Aligned_vector<std::byte, 16>& cb)
{
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());
   const auto uint_value = static_cast<std::uint32_t>(value.as<bool>());

   set_constant(cb, cb_offset, uint_value);
}

template<typename Int>
void process_int_prop(const YAML::Node& value, const YAML::Node& desc,
                      const std::size_t count, Aligned_vector<std::byte, 16>& cb)
{
   static_assert(sizeof(Int) == 4);

   const std::array<Int, 2> range{desc["Range"s][0].as<Int>(),
                                  desc["Range"s][1].as<Int>()};
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());

   for (auto i = 0; i < count; ++i) {
      const auto clamped_value = std::clamp(value[i].as<Int>(), range[0], range[1]);

      set_constant(cb, cb_offset + i, clamped_value);
   }
}
template<typename Int>
void process_int_prop(const YAML::Node& value, const YAML::Node& desc,
                      Aligned_vector<std::byte, 16>& cb)
{
   static_assert(sizeof(Int) == 4);

   const std::array<Int, 2> range{desc["Range"s][0].as<Int>(),
                                  desc["Range"s][1].as<Int>()};
   const auto cb_offset = read_constant_offset(desc["Constant"s].as<std::string>());

   const auto clamped_value = std::clamp(value.as<Int>(), range[0], range[1]);

   set_constant(cb, cb_offset, clamped_value);
}

void process_prop(const YAML::Node& value, const YAML::Node& desc,
                  Aligned_vector<std::byte, 16>& cb)
{
   if (const auto type = desc["Type"s].as<std::string>(); type == "float"sv) {
      process_float_prop(value, desc, cb);
   }
   else if (begins_with(type, "vec"sv)) {
      process_float_prop(value, desc, get_vec_size(type), cb);
   }
   else if (type == "bool"sv) {
      process_bool_prop(value, desc, cb);
   }
   else if (begins_with(type, "bvec"sv)) {
      process_bool_prop(value, desc, get_vec_size(type), cb);
   }
   else if (type == "int"sv) {
      process_int_prop<std::int32_t>(value, desc, cb);
   }
   else if (begins_with(type, "ivec"sv)) {
      process_int_prop<std::int32_t>(value, desc, get_vec_size(type), cb);
   }
   else if (type == "uint"sv) {
      process_int_prop<std::uint32_t>(value, desc, cb);
   }
   else if (begins_with(type, "uvec"sv)) {
      process_int_prop<std::uint32_t>(value, desc, get_vec_size(type), cb);
   }
}

auto init_cb_with_defaults(const YAML::Node& property_mappings)
   -> Aligned_vector<std::byte, 16>
{
   Aligned_vector<std::byte, 16> cb;
   cb.reserve(256);

   for (auto& prop : property_mappings) {
      try {
         process_prop(prop.second["Default"s], prop.second, cb);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>(
            "Error occured while processing rendertype property "sv,
            std::quoted(prop.first.as<std::string>()), ": "sv, e.what());
      }
   }

   return cb;
}

void read_prop(const std::string& prop_key, const YAML::Node& value,
               const YAML::Node& property_mappings, Aligned_vector<std::byte, 16>& cb)
{
   auto desc = property_mappings[prop_key];

   if (!desc) {
      throw compose_exception<std::runtime_error>("Undescribed material property "sv,
                                                  std::quoted(prop_key),
                                                  " encountered."sv);
   }

   process_prop(value, desc, cb);
}

void read_texture_mapping(const std::string& texture_key, const YAML::Node& value,
                          const YAML::Node& texture_mappings,
                          Material_info& material_info)
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

auto describe_material(std::string_view name, const YAML::Node& description,
                       const YAML::Node& material) -> Material_info
{
   Material_info info{};

   info.name = name;
   info.rendertype = material["RenderType"s].as<std::string>();
   info.overridden_rendertype = rendertype_from_string(
      description["Overridden RenderType"s].as<std::string>());
   info.fail_safe_texture_index =
      description["Failsafe Texture Index"s].as<std::uint32_t>();
   info.constant_buffer = init_cb_with_defaults(description["property_mappings"s]);

   for (auto& prop : material["Material"s]) {
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

   for (auto& texture : material["Textures"s]) {
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
