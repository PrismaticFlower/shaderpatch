#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>
#include <vector>

#include <gsl/gsl>

namespace sp {

template<typename To, typename From>
inline To bit_cast(const From from) noexcept
{
   static_assert(sizeof(From) <= sizeof(To));
   static_assert(std::is_standard_layout_v<To>);
   static_assert(std::is_trivially_copyable_v<From>);

   To to;

   std::memcpy(&to, &from, sizeof(From));

   return to;
}

template<typename To>
inline To bit_cast(const std::span<const std::byte> from) noexcept
{
   static_assert(std::is_standard_layout_v<To>);

   Expects(sizeof(To) <= from.size());

   To to;

   std::memcpy(&to, from.data(), sizeof(to));

   return to;
}

template<typename To>
inline To bit_cast(const std::span<std::byte> from) noexcept
{
   return bit_cast<To>(std::span<const std::byte>{from});
}

template<typename From>
inline auto make_vector(const From& from) noexcept
   -> std::vector<typename From::value_type>
{
   return {std::cbegin(from), std::cend(from)};
}

template<auto multiple>
constexpr auto next_multiple_of(const decltype(multiple) value) noexcept
   -> decltype(multiple)
{
   const auto remainder = value % multiple;

   if (remainder != 0) return value + (multiple - remainder);

   return value;
}

template<typename Type>
constexpr auto next_multiple_of(const Type multiple, const Type value) noexcept
   -> decltype(multiple)
{
   const auto remainder = value % multiple;

   if (remainder != 0) return value + (multiple - remainder);

   return value;
}

template<auto multiple>
constexpr bool is_multiple_of(const decltype(multiple) value) noexcept
{
   return (value % multiple) == 0;
}

constexpr auto next_power_of_2(std::uint32_t value) -> std::uint32_t
{
   value--;
   value |= value >> 1u;
   value |= value >> 2u;
   value |= value >> 4u;
   value |= value >> 8u;
   value |= value >> 16u;
   value++;

   return value;
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

template<typename Type, typename Alignment>
struct Aligned_allocator {
   static_assert(Alignment::value >= alignof(Type));

   using value_type = Type;
   using is_always_equal = std::true_type;

   Aligned_allocator() noexcept = default;

   template<typename Other>
   Aligned_allocator(const Aligned_allocator<Other, Alignment>&) noexcept
   {
   }

   [[nodiscard]] auto allocate(const std::size_t count) const noexcept -> Type*
   {
      auto* const result = _aligned_malloc(sizeof(Type) * count, Alignment::value);

      if (!result) std::terminate();

      return static_cast<Type*>(result);
   }

   void deallocate(Type* const mem, const std::size_t) const noexcept
   {
      _aligned_free(mem);
   }
};

template<typename L, typename La, typename R, typename Ra>
constexpr bool operator==(const Aligned_allocator<L, La>,
                          const Aligned_allocator<R, Ra>) noexcept
{
   return true;
}

template<typename L, typename La, typename R, typename Ra>
constexpr bool operator!=(const Aligned_allocator<L, La>,
                          const Aligned_allocator<R, Ra>) noexcept
{
   return false;
}

template<typename Type, std::size_t alignment>
using Aligned_allocator_for =
   Aligned_allocator<Type, std::integral_constant<std::size_t, alignment>>;

template<typename Type, std::size_t alignment>
using Aligned_vector = std::vector<Type, Aligned_allocator_for<Type, alignment>>;

template<std::size_t alignment, typename From>
inline auto make_aligned_vector(const From& from) noexcept
   -> Aligned_vector<typename From::value_type, alignment>
{
   return {std::cbegin(from), std::cend(from)};
}

template<typename... Args>
inline auto safe_min(Args&&... args)
{
   auto result = std::min(std::forward<Args>(args)...);

   return result;
}

template<typename... Args>
inline auto safe_max(Args&&... args)
{
   auto result = std::max(std::forward<Args>(args)...);

   return result;
}
}
