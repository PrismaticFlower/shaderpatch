#pragma once

#include <memory>
#include <utility>

namespace sp {

template<std::size_t alignment>
class Linear_allocator {
public:
   Linear_allocator(std::size_t max_size) : _size{align_up(max_size)} {}

   [[nodiscard]] auto allocate(const std::size_t size) noexcept -> std::byte*
   {
      const auto aligned_size = align_up(size);
      const auto allocation_offset = std::exchange(_head, _head + aligned_size);

      if (_head > _size) [[unlikely]] {
         return nullptr;
      }

      return reinterpret_cast<std::byte*>(_storage.get()) + allocation_offset;
   }

   auto used_bytes() const noexcept -> std::size_t
   {
      return _head;
   }

   void reset() noexcept
   {
      _head = 0;
   }

private:
   static auto align_up(std::size_t value) -> std::size_t
   {
      return (value + alignment - 1) / alignment * alignment;
   }

   std::size_t _head = 0;
   const std::size_t _size;
   const std::unique_ptr<std::aligned_storage_t<alignment, alignment>[]> _storage =
      std::make_unique_for_overwrite<std::aligned_storage_t<alignment, alignment>[]>(
         _size / alignment);
};

}
