#pragma once

#include "enum_flags.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace sp {

enum class Vertex_shader_flags : std::uint32_t {
   none = 0b0,
   compressed = 0b1,
   position = 0b10,
   normal = 0b100,
   tangents = 0b1000,
   texcoords = 0b10000,
   color = 0b100000,
   hard_skinned = 0b1000000
};

enum class Shader_flags : std::uint32_t {
   none = 0,
   light_point = 1,
   light_point2 = 2,
   light_point4 = 3,
   light_spot = 4,
   light_dir = 16,
   // soft_skinned = 32, // Soft Skinned
   skinned = 64,     // Hard Skinned
   vertexcolor = 128 // Vertexcolor
};

enum class Pass_flags : std::uint32_t {
   none = 0,
   nolighting = 1,
   lighting = 2,

   notransform = 16,
   position = 32,
   normal = 64,
   tangents = 128,
   vertex_color = 256,
   texcoords = 512
};

template<>
struct is_enum_flag<Vertex_shader_flags> : std::true_type {
};

template<>
struct is_enum_flag<Shader_flags> : std::true_type {
};
template<>
struct is_enum_flag<Pass_flags> : std::true_type {
};

template<typename Type>
constexpr bool is_shader_flag_v =
   std::is_same_v<Type, Vertex_shader_flags> ||
   std::is_same_v<Type, Shader_flags> || std::is_same_v<Type, Pass_flags>;

constexpr bool pass_flag_set(const Pass_flags& flags, const Pass_flags& test_flag) noexcept
{
   return (flags & test_flag) == test_flag;
}

inline auto get_pass_flags(std::string base_input, bool lighting,
                           bool vertex_color, bool texcoords) -> sp::Pass_flags
{
   using namespace std::literals;

   Pass_flags flags = Pass_flags::none;

   if (lighting) {
      flags |= Pass_flags::lighting;
   }
   else {
      flags |= Pass_flags::nolighting;
   }

   if (vertex_color) flags |= Pass_flags::vertex_color;
   if (texcoords) flags |= Pass_flags::texcoords;

   if (base_input == "none"sv) {
      flags |= Pass_flags::notransform;

      return flags;
   }
   else if (base_input == "position"sv) {
      flags |= Pass_flags::position;
   }
   else if (base_input == "normals"sv) {
      flags |= Pass_flags::position;
      flags |= Pass_flags::normal;
   }
   else if (base_input == "tangents"sv) {
      flags |= Pass_flags::position;
      flags |= Pass_flags::normal;
      flags |= Pass_flags::tangents;
   }
   else {
      throw std::runtime_error{"Invalid base_input value: "s += base_input};
   }

   return flags;
}

inline auto to_string(const Vertex_shader_flags flags) noexcept
{
   using namespace std::literals;

   if (flags == Vertex_shader_flags::none) return "none"s;

   std::string result;

   bool first = true;

   const auto add_flag = [&](const Vertex_shader_flags flag, const std::string_view str) {
      if ((flags & flag) != Vertex_shader_flags::none) {
         if (not std::exchange(first, false)) result += " | "sv;

         result += str;
      }
   };

   add_flag(Vertex_shader_flags::compressed, "compressed"sv);
   add_flag(Vertex_shader_flags::position, "position"sv);
   add_flag(Vertex_shader_flags::normal, "normal"sv);
   add_flag(Vertex_shader_flags::tangents, "tangents"sv);
   add_flag(Vertex_shader_flags::texcoords, "texcoords"sv);
   add_flag(Vertex_shader_flags::color, "color"sv);
   add_flag(Vertex_shader_flags::hard_skinned, "hard_skinned"sv);

   return result;
}

}
