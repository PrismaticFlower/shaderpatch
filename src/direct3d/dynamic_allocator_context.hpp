#pragma once

#include "linear_allocator.hpp"

namespace sp::d3d9 {

class Dynamic_allocator_context {
public:
   [[nodiscard]] auto allocate(const std::size_t size) noexcept -> std::byte*
   {
      return _allocator.allocate(size);
   }

   void reset() noexcept
   {
      _allocator.reset();
   }

private:
   constexpr static std::size_t allocator_size = 4'194'304; // 2^22

   Linear_allocator<16> _allocator{allocator_size};
};

}
