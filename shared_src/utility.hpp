#pragma once

#include <cstddef>
#include <cstring>
#include <iterator>
#include <malloc.h>
#include <vector>

namespace sp {

template<std::size_t alignment>
class Aligned_scratch_buffer {
public:
   Aligned_scratch_buffer() noexcept = default;

   ~Aligned_scratch_buffer()
   {
      if (_buffer) _aligned_free(_buffer);
   }

   Aligned_scratch_buffer(const Aligned_scratch_buffer&) = delete;
   Aligned_scratch_buffer& operator=(const Aligned_scratch_buffer&) = delete;

   Aligned_scratch_buffer(Aligned_scratch_buffer&&) = delete;
   Aligned_scratch_buffer& operator=(Aligned_scratch_buffer&&) = delete;

   void resize(const std::size_t new_size) noexcept
   {
      if (_size == new_size) return;
      if (new_size == 0) {
         _buffer = nullptr;
         _size = 0;

         return;
      }

      _buffer = static_cast<std::byte*>(_aligned_malloc(new_size, alignment));

      if (!_buffer) std::terminate();

      _size = new_size;
   }

   void ensure_min_size(const std::size_t required_size) noexcept
   {
      if (_size >= required_size) return;

      resize(required_size);
   }

   void ensure_within_size(const std::size_t max_size) noexcept
   {
      if (_size <= max_size) return;

      resize(max_size);
   }

   std::size_t size() const noexcept
   {
      return _size;
   }

   std::byte* data() noexcept
   {
      return _buffer;
   }

   const std::byte* data() const noexcept
   {
      return _buffer;
   }

private:
   std::byte* _buffer = nullptr;
   std::size_t _size = 0;
};

template<typename To, typename From>
inline To bit_cast(const From from)
{
   static_assert(sizeof(From) <= sizeof(To));
   static_assert(std::is_trivial_v<To>);
   static_assert(std::is_trivially_copyable_v<From>);

   To to;

   std::memcpy(&to, &from, sizeof(From));

   return to;
}

template<typename From>
inline auto make_vector(const From& from) noexcept
   -> std::vector<typename From::value_type>
{
   return {std::cbegin(from), std::cend(from)};
}

}
