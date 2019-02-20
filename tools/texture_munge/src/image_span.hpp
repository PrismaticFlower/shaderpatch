#pragma once

#include "utility.hpp"

#include <cstddef>
#include <execution>
#include <optional>

#include <glm/glm.hpp>

#include <DirectXTex.h>

namespace sp {

class Image_span {
public:
   using value_type = glm::vec4;
   using index_type = glm::ivec2;

   Image_span(const glm::ivec2 size, const std::size_t row_pitch,
              std::byte* const data, const DXGI_FORMAT format) noexcept;

   Image_span(const DirectX::Image& image) noexcept;

   auto load(const index_type index) const noexcept -> value_type;

   void store(const index_type index, const value_type value) noexcept;

   auto exchange(const index_type index, const value_type new_value) noexcept
      -> value_type;

   auto subspan(const index_type offset, const index_type length) const noexcept
      -> Image_span;

   auto subspan_rows(const index_type::value_type row,
                     const index_type::value_type count) const noexcept -> Image_span;

   auto size() const noexcept -> glm::ivec2;

private:
   const glm::ivec2 _bounds;
   const std::size_t _row_pitch;
   std::byte* const _data;

   using Load_value = auto(const glm::ivec2 index, const std::size_t row_pitch,
                           const std::byte* const data) noexcept -> glm::vec4;
   using Store_value = void(const glm::vec4 value, const glm::ivec2 index,
                            const std::size_t row_pitch, std::byte* const data) noexcept;

   Load_value& _load_func;
   Store_value& _store_func;

   const DXGI_FORMAT _format;
};

// Helper function for iterating through an Image_span across multiple threads in a cache-friendly manner.
// `func` will be called with the index of the current texel being processed.
template<typename Policy, typename Func>
inline void for_each(Policy&& policy, const Image_span& span, Func func) noexcept
{
   const auto threads = std::thread::hardware_concurrency();
   const auto work_size = span.size().y / threads;

   if (work_size) {
      std::for_each_n(std::forward<Policy>(policy), Index_iterator{}, threads,
                      [work_size, span_x_size = span.size().x, &func](const auto item) {
                         const auto offset = item * work_size;

                         for (auto y = 0; y < work_size; ++y) {
                            for (auto x = 0; x < span_x_size; ++x) {
                               func(glm::ivec2{x, offset + y});
                            }
                         }
                      });
   }

   // process remainder
   const auto remainder_offset = work_size * threads;
   const auto remainder_size = span.size().y - remainder_offset;

   if (remainder_size) {
      for (auto y = 0; y < remainder_size; ++y) {
         for (auto x = 0; x < span.size().x; ++x) {
            func(glm::ivec2{x, remainder_offset + y});
         }
      }
   }
}

}
