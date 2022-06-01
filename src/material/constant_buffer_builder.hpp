#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>

namespace sp::material {

class Constant_buffer_builder {
public:
   using value_type =
      std::variant<float, glm::vec2, glm::vec3, glm::vec4, // float values
                   std::int32_t, glm::ivec2, glm::ivec3, glm::ivec4, // int values
                   std::uint32_t, glm::uvec2, glm::uvec3, glm::uvec4, // uint values
                   bool // bool values
                   >;

   constexpr static std::size_t buffer_size_alignment = 256;

   explicit Constant_buffer_builder(const std::string_view layout) noexcept;

   void set_int(const std::string_view field_name, const std::int32_t value) noexcept
   {
      set(field_name, value);
   }

   void set_uint(const std::string_view field_name, const std::uint32_t value) noexcept
   {
      set(field_name, value);
   }

   void set(const std::string_view field_name, const value_type& value) noexcept;

   auto complete() noexcept -> std::vector<std::byte>
   {
      _field_mapping.clear();

      return std::move(_backing_storage);
   }

private:
   template<typename... Args>
   static auto value_ptr_type_helper(std::variant<Args...>)
      -> std::variant<Args* const...>;

   using value_ptr_type = std::invoke_result_t<decltype(
      []() { return value_ptr_type_helper(value_type{}); })>;

   absl::flat_hash_map<std::string, value_ptr_type> _field_mapping;
   std::vector<std::byte> _backing_storage;
};

}
