
#include "constant_buffer_builder.hpp"
#include "../logger.hpp"
#include "overloaded.hpp"
#include "string_utilities.hpp"
#include "utility.hpp"

#include <charconv>
#include <typeinfo>

#include <fmt/format.h>

using namespace std::literals;

namespace sp::material {

namespace {

enum class Type {
   float_,
   float2,
   float3,
   float4,
   int_,
   int2,
   int3,
   int4,
   uint,
   uint2,
   uint3,
   uint4,
   bool_
};

auto type_size(const Type type) -> std::size_t
{
   switch (type) {
   case Type::float_:
   case Type::int_:
   case Type::uint:
   case Type::bool_:
      return 4;
   case Type::float2:
   case Type::int2:
   case Type::uint2:
      return 8;
   case Type::float3:
   case Type::int3:
   case Type::uint3:
      return 12;
   case Type::float4:
   case Type::int4:
   case Type::uint4:
      return 16;
   }

   std::terminate();
}

struct Pack_result {
   std::size_t offset = 0;
   std::size_t size = 0;
};

auto pack_type(const Type type, const std::size_t size, const bool array_item) -> Pack_result
{
   const std::size_t alignment = 16;
   const std::size_t aligned_size = next_multiple_of<alignment>(size);

   if (array_item) {
      return {.offset = aligned_size, .size = aligned_size + type_size(type)};
   }

   if (is_multiple_of<alignment>(size)) {
      return {.offset = size, .size = size + type_size(type)};
   }

   if ((aligned_size - size) >= type_size(type)) {
      return {.offset = size, .size = size + type_size(type)};
   }

   return {.offset = aligned_size, .size = aligned_size + type_size(type)};
}

auto parse_type(const std::string_view type_str) noexcept -> Type
{
   absl::flat_hash_map<std::string_view, Type> types =
      {{"float"sv, Type::float_},  {"float1"sv, Type::float_},
       {"float2"sv, Type::float2}, {"float3"sv, Type::float3},
       {"float4"sv, Type::float4}, {"int"sv, Type::int_},
       {"int1"sv, Type::int_},     {"int2"sv, Type::int2},
       {"int3"sv, Type::int3},     {"int4"sv, Type::int4},
       {"uint"sv, Type::uint},     {"uint1"sv, Type::uint},
       {"uint2"sv, Type::uint2},   {"uint3"sv, Type::uint3},
       {"uint4"sv, Type::uint4},   {"bool"sv, Type::bool_}};

   auto type = types.find(type_str);

   if (type == types.end()) {
      log_and_terminate("Invalid type in constant buffer layout '{}'!"sv, type_str);
   }

   return type->second;
}

template<typename T>
void parse_layout(const std::string_view layout, T&& callback) noexcept
{
   for (auto line : Lines_iterator{layout}) {
      auto string = trim_whitespace(line.string);

      if (string.empty()) continue;

      string = split_string_on(string, "//"sv)[0];

      if (string.empty()) continue;

      auto [type_string, var_name] = split_string_on(string, " "sv);

      type_string = trim_whitespace(type_string);
      var_name = split_string_on(var_name, ";"sv)[0];
      var_name = trim_whitespace(var_name);

      if (type_string.empty() || var_name.empty()) {
         log_and_terminate("Parse error on line #{} in constant buffer layout!"sv,
                           line.number);
      }

      if (auto [array_var_name, size_str] = split_string_on(var_name, "["sv);
          !size_str.empty()) {
         size_str = split_string_on(size_str, "]"sv)[0];
         size_str = trim_whitespace(size_str);

         std::size_t size = 0;

         if (std::from_chars(size_str.data(), size_str.data() + size_str.size(), size)
                .ec != std::errc{}) {
            log_and_terminate("Parse error on line #{} in constant buffer layout! Unable to parse array size '{}'."sv,
                              line.number, size_str);
         }

         for (std::size_t i = 0; i < size; ++i) {
            callback(fmt::format("{}[{}]"sv, array_var_name, i),
                     parse_type(type_string), true);
         }
      }
      else {
         callback(var_name, parse_type(type_string), false);
      }
   }
}

}

Constant_buffer_builder::Constant_buffer_builder(const std::string_view layout) noexcept
{
   std::size_t size = 0;

   parse_layout(layout, [&](const std::string_view, const Type type, const bool array_item) {
      size = pack_type(type, size, array_item).size;
   });

   _backing_storage.resize(next_multiple_of<buffer_size_alignment>(size));

   size = 0;

   parse_layout(layout, [&](const std::string_view field, const Type type,
                            const bool array_item) {
      auto packed = pack_type(type, size, array_item);
      size = packed.size;

      switch (type) {
      case Type::float_:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset]) float);
         break;
      case Type::float2:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset]) glm::vec2);
         break;
      case Type::float3:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset]) glm::vec3);
         break;
      case Type::float4:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset]) glm::vec4);
         break;
      case Type::int_:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          std::int32_t);
         break;
      case Type::int2:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::ivec2);
         break;
      case Type::int3:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::ivec3);
         break;
      case Type::int4:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::ivec4);
         break;
      case Type::uint:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          std::uint32_t);
         break;
      case Type::uint2:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::uvec2);
         break;
      case Type::uint3:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::uvec3);
         break;
      case Type::uint4:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset])
                                          glm::uvec4);
         break;
      case Type::bool_:
         _field_mapping.emplace(field, new (&_backing_storage[packed.offset]) bool);
         break;
      }
   });
}

void Constant_buffer_builder::set(const std::string_view field_name,
                                  const value_type& value) noexcept
{
   if (auto field = _field_mapping.find(field_name); field != _field_mapping.end()) {
      // clang-format off
      std::visit(
         overloaded{
            []<typename D, typename S, typename = std::enable_if_t<std::is_same_v<D, S>>>(
               D* const dest, const S& src) { *dest = src; },
            [&]<typename D, typename S,
                typename = std::enable_if_t<!std::is_same_v<D, S>>>(D* const, const S&) {
               log_and_terminate("Type mismatch (field type: {} arg type: {}) for material constant buffer field '{}'!"sv,
                                     typeid(D).name(), typeid(S).name(), field_name);
            },
         },
         field->second, value);
   }
   else {
      log_and_terminate("Attempt to set nonexistent material constant buffer field '{}'!"sv,
                            field_name);
   }
}
// clang-format on
}
