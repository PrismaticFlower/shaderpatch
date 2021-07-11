#pragma once

#include <memory>

namespace sp {

template<std::size_t alignment>
class Linear_allocator {
public:
   Linear_allocator(std::size_t max_size)
   {
      const auto aligned_size = align_up(max_size, alignment);

      _storage =
         std::make_unique_for_overwrite<std::aligned_storage_t<alignment, alignment>[]>(
            aligned_size);
      _head = reinterpret_cast<std::byte*>(_storage.get());
      _end = reinterpret_cast<std::byte*>(_storage.get()) + aligned_size;
   }

   [[nodiscard]] auto allocate(const std::size_t size) noexcept -> std::byte*
   {
      const auto aligned_size = align_up(size, alignment);

      auto* const data = (_head += aligned_size);

      if (data >= _end) return nullptr;

      return data;
   }

   void reset() noexcept
   {
      _head = reinterpret_cast<std::byte*>(_storage.get());
   }

private:
   static auto align_up(std::size_t value, std::size_t alignment) -> std::size_t
   {
      return (value + alignment - 1) / alignment * alignment;
   }

   std::byte* _head = nullptr;
   std::byte* _end = nullptr;
   std::unique_ptr<std::aligned_storage_t<alignment, alignment>[]> _storage;
};

}
