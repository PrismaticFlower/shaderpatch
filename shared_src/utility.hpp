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

struct Index_iterator {
   using difference_type = std::ptrdiff_t;
   using value_type = const difference_type;
   using pointer = value_type*;
   using reference = value_type&;
   using iterator_category = std::random_access_iterator_tag;

   Index_iterator() = default;
   ~Index_iterator() = default;

   Index_iterator(const Index_iterator&) = default;
   Index_iterator& operator=(const Index_iterator&) = default;
   Index_iterator(Index_iterator&&) = default;
   Index_iterator& operator=(Index_iterator&&) = default;

   auto operator*() const noexcept -> value_type
   {
      return _index;
   }

   auto operator++() noexcept -> Index_iterator&
   {
      ++_index;

      return *this;
   }

   auto operator++(int) const noexcept -> Index_iterator
   {
      auto copy = *this;

      ++copy._index;

      return copy;
   }

   auto operator--() noexcept -> Index_iterator&
   {
      --_index;

      return *this;
   }

   auto operator--(int) const noexcept -> Index_iterator
   {
      auto copy = *this;

      --copy._index;

      return copy;
   }

   auto operator[](const difference_type offset) const noexcept -> value_type
   {
      return _index + offset;
   }

   auto operator+=(const difference_type offset) noexcept -> Index_iterator&
   {
      _index += offset;

      return *this;
   }

   auto operator-=(const difference_type offset) noexcept -> Index_iterator&
   {
      _index -= offset;

      return *this;
   }

   friend bool operator==(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator!=(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator<(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator<=(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator>(const Index_iterator& left, const Index_iterator& right) noexcept;

   friend bool operator>=(const Index_iterator& left, const Index_iterator& right) noexcept;

private:
   difference_type _index{};
};

inline auto operator+(const Index_iterator& iter,
                      const Index_iterator::difference_type offset) noexcept -> Index_iterator
{
   auto copy = iter;

   return copy += offset;
}

inline auto operator+(const Index_iterator::difference_type offset,
                      const Index_iterator& iter) noexcept -> Index_iterator
{
   return iter + offset;
}

inline auto operator-(const Index_iterator& iter,
                      const Index_iterator::difference_type offset) noexcept -> Index_iterator
{
   auto copy = iter;

   return copy -= offset;
}

inline auto operator-(const Index_iterator::difference_type offset,
                      const Index_iterator& iter) noexcept -> Index_iterator
{
   return iter - offset;
}

inline auto operator-(const Index_iterator& left, const Index_iterator& right) noexcept
   -> Index_iterator::difference_type
{
   return *left - *right;
}

inline bool operator==(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left == *right;
}

inline bool operator!=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left != *right;
}

inline bool operator<(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left < *right;
}

inline bool operator<=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left <= *right;
}

inline bool operator>(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left > *right;
}

inline bool operator>=(const Index_iterator& left, const Index_iterator& right) noexcept
{
   return *left >= *right;
}

}
