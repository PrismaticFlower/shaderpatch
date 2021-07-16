#pragma once

#include "linear_allocator.hpp"

#include <array>
#include <atomic>

namespace sp {

class Constants_indirect_storage_allocator {
public:
   constexpr static std::size_t alignment = 16;
   constexpr static std::size_t max_allocators = 3;

   static_assert(max_allocators > 1, "max_allocators must be greater than 1!");

   Constants_indirect_storage_allocator(const std::size_t size)
      : _allocators{Allocator{size}, Allocator{size}, Allocator{size}}
   {
      _allocator = &_allocators[0];
      _last_used = &_allocators_last_used[0];
   }

   [[nodiscard]] auto allocate(const std::size_t size) noexcept -> std::byte*
   {
      return _allocator->allocate(size);
   }

   [[nodiscard]] auto used_bytes() const noexcept -> std::size_t
   {
      return _allocator->used_bytes();
   }

   void reset(std::uint64_t current_recorded_frame,
              std::atomic_uint64_t& completed_frame) noexcept
   {
      *_last_used = current_recorded_frame;

      const auto new_index = next_allocator_index();

      _allocator = &_allocators[new_index];
      _last_used = &_allocators_last_used[new_index];

      wait_for_ready(completed_frame);

      _allocator->reset();
   }

private:
   using Allocator = Linear_allocator<alignment>;

   auto current_index() const noexcept -> std::size_t
   {
      for (std::size_t i = 0; i < max_allocators; ++i) {
         if (&_allocators[i] == _allocator) return i;
      }

      std::terminate();
   }

   auto next_allocator_index() const noexcept -> std::size_t
   {
      return (current_index() + 1) % max_allocators;
   }

   void wait_for_ready(std::atomic_uint64_t& completed_frame) const noexcept
   {
      auto current_completed = completed_frame.load();

      while (current_completed < *_last_used) {
         completed_frame.wait(current_completed);

         current_completed = completed_frame.load();
      }
   }

   Allocator* _allocator;
   std::uint64_t* _last_used;

   std::array<Allocator, max_allocators> _allocators;
   std::array<std::uint64_t, max_allocators> _allocators_last_used{0, 0, 0};
};

};
