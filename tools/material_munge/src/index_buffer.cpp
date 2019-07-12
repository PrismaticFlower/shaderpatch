
#include "index_buffer.hpp"

#include <gsl/gsl>

namespace sp {

namespace {

constexpr bool is_degenerate_triangle(const std::array<std::uint16_t, 3> triangle) noexcept
{
   return triangle[0] == triangle[1] || triangle[0] == triangle[2] ||
          triangle[1] == triangle[2];
}

}

auto create_index_buffer(ucfb::Reader_strict<"IBUF"_mn> ibuf) -> Index_buffer_16
{
   const auto index_count = ibuf.read<std::uint32_t>();

   if (index_count < 3) return {};

   std::vector<std::array<std::uint16_t, 3>> index_buffer;
   index_buffer.reserve(index_count);

   auto strips = ibuf.read_array<std::uint16_t>(index_count);

   for (auto i = 0; i < (index_count - 2); ++i) {
      const bool even = (i & 1) == 0;
      const auto tri = even ? std::array{strips[i], strips[i + 1], strips[i + 2]}
                            : std::array{strips[i + 2], strips[i + 1], strips[i]};

      if (is_degenerate_triangle(tri)) continue;

      index_buffer.emplace_back(tri);
   }

   return index_buffer;
}

auto shrink_index_buffer(const Index_buffer_32& fat_ibuf) -> Index_buffer_16
{
   Index_buffer_16 new_ibuf;
   new_ibuf.reserve(fat_ibuf.capacity());

   for (const auto& tri : fat_ibuf) {
      new_ibuf.push_back({gsl::narrow<std::uint16_t>(tri[0]),
                          gsl::narrow<std::uint16_t>(tri[1]),
                          gsl::narrow<std::uint16_t>(tri[2])});
   }

   return new_ibuf;
}

}
