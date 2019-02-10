#pragma once

#include "enum_flags.hpp"

#include <cstdint>
#include <string>
#include <string_view>

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

enum class Pixel_shader_flags : std::uint32_t {
   none = 0b0,
   light_point_1 = 0b1,
   light_point_2 = 0b10,
   light_point_4 = 0b100,
   light_spot = 0b1000,
   light_directional = 0b10000
};

enum class Shader_flags : std::uint32_t {
   none = 0,

   light_dir = 16,   // Directional Light
   light_point = 1,  // Point Light
   light_point2 = 2, // 2 Point Lights
   light_point4 = 3, // 4 Point Lights
   light_spot = 4,   // Spot Light

   light_dir_point = 17,      // Directional Lights + Point Light
   light_dir_point2 = 18,     // Directional Lights + 2 Point Lights
   light_dir_point4 = 19,     // Directional Lights + 4 Point Lights
   light_dir_spot = 20,       // Direction Lights + Spot Light
   light_dir_point_spot = 21, // Directional Lights + Point Light + Spot Light
   light_dir_point2_spot = 22, // Directional Lights + 2 Point Lights + Spot Light

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
struct is_enum_flag<Pixel_shader_flags> : std::true_type {
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
   std::is_same_v<Type, Pixel_shader_flags> ||
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

}
