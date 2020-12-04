#pragma once

#include "enum_flags.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace sp::shader {

enum class Stage { compute, vertex, hull, domain, geometry, pixel };

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

constexpr bool marked_as_enum_flag(Vertex_shader_flags)
{
   return true;
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
