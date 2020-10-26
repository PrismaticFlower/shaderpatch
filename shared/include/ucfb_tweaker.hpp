#pragma once

#include "magic_number.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gsl/gsl>

namespace sp::ucfb {

template<typename Type>
class Tweaker_proxy {
public:
   static_assert(std::is_trivially_copyable_v<Type>,
                 "Type must be trivially copyable.");
   static_assert(!std::is_reference_v<Type>, "Type can not be a reference.");
   static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

   Type load() const noexcept
   {
      Type val;

      std::memcpy(&val, _memory, sizeof(Type));

      return val;
   }

   void store(const Type value) noexcept
   {
      std::memcpy(_memory, &value, sizeof(Type));
   }

   friend class Tweaker;

private:
   explicit Tweaker_proxy(std::byte* memory) noexcept : _memory{memory} {}

   std::byte* _memory;
};

class Tweaker {
public:
   Tweaker() = delete;

   //! \brief Creates a Tweaker from a span of memory.
   //!
   //! \param bytes The span of memory holding the root chunk. The size of the
   //! span must be at least 8.
   //!
   //! \exception std::runtime_error Thrown when the size of the chunk does not
   //! match the size of the span.
   Tweaker(const std::span<std::byte> bytes)
      : _mn{reinterpret_cast<const Magic_number&>(bytes[0])},
        _data{&bytes[8], reinterpret_cast<const std::uint32_t&>(bytes[4])}
   {
      Expects((bytes.size() >= 8));

      if (_data.size() > bytes.size() - 8) {
         throw std::runtime_error{
            "Size of supplied memory is less than size of supposed chunk."};
      }
   }

   //! \brief Creates a Tweaker from a magic number and a span of memory.
   //! The span of memory excludes the magic number and size tag of the chunk.
   //!
   //! \param mn The magic number of the chunk.
   //! \param bytes The contents of the chunk.
   Tweaker(const Magic_number mn, const std::span<std::byte> bytes)
      : _mn{mn}, _data{bytes}
   {
   }

   //! \brief Fetches a child chunk.
   //!
   //! \param unaligned If the get is unaligned or not.
   //!
   //! \return A new Tweaker for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when getting the child would go past
   //! the end of the current chunk.
   Tweaker get_child(const bool unaligned = false)
   {
      const auto child_offset = _head;
      const auto child_mn = get<Magic_number>().load();
      const auto child_size = get<std::uint32_t>().load();

      _head += child_size;

      check_head();

      if (!unaligned) align_head();

      return Tweaker{_data.subspan(child_offset, child_size + 8)};
   }

   //! \brief Fetches an unaligned child chunk.
   //!
   //! \return A new Tweaker for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when getting the child would go past
   //! the end of the current chunk.
   Tweaker get_child_unaligned()
   {
      return get_child(true);
   }

   //! \brief Fetches a child if it's magic number matches an expected
   //! magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A new Tweaker for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when the magic number of the child
   //! and the expected magic number do not match. If this happens the read head
   //! is not moved. \exception std::runtime_error Thrown when getting the child
   //! would go past the end of the current chunk.
   template<Magic_number type_mn>
   Tweaker get_child_checked(const bool unaligned = false)
   {
      const auto cur_head = _head;

      const auto child = get_child(unaligned);

      if (child.magic_number() != type_mn) {
         throw std::runtime_error{
            "Chunk magic number mistmatch"
            " when performing checked fetch of child chunk."};
      }

      return child;
   }

   //! \brief Fetches an unaligned child if it's magic number matches an
   //! expected magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \return A new Tweaker_strict<type_mn> for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when the magic number of the child
   //! and the expected magic number do not match. If this happens the read head
   //! is not moved. \exception std::runtime_error Thrown when getting the child
   //! would go past the end of the current chunk.
   template<Magic_number type_mn>
   Tweaker get_child_checked_unaligned()
   {
      return get_child_checked<type_mn>(true);
   }

   //! \brief Gets a reference to a trivial value from the chunk.
   //!
   //! \tparam Type The type of the value to get. The type must be trivially copyable.
   //!
   //! \param unaligned If the get is unaligned or not.
   //!
   //! \return A proxy to the value.
   //!
   //! \exception std::runtime_error Thrown when getting the value would go past the end
   //! of the chunk.
   template<typename Type>
   auto get(const bool unaligned = false) -> Tweaker_proxy<Type>
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable.");
      static_assert(!std::is_reference_v<Type>, "Type can not be a reference.");
      static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

      const auto cur_pos = _head;
      _head += sizeof(Type);

      check_head();

      if (!unaligned) align_head();

      return Tweaker_proxy<Type>(&_data[cur_pos]);
   }

   //! \brief Gets a reference to a trivial unaligned value from the chunk.
   //!
   //! \tparam Type The type of the value to get. The type must be trivially copyable.
   //!
   //! \return A proxy to the value.
   //!
   //! \exception std::runtime_error Thrown when getting the value would go past the end
   //! of the chunk.
   template<typename Type>
   auto get_unaligned() -> Tweaker_proxy<Type>
   {
      return get<Type>(true);
   }

   //! \brief Reads a null-terminated string from a chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A std::string_view to the string, excluding the null terminator.
   //!
   //! \exception std::runtime_error Thrown when reading the string would go
   //! past the end of the chunk.
   //! \exception std::runtime_error Thrown when reading the string would
   //! return a string that is not null-terminated.
   auto read_string(const bool unaligned = false) -> std::string_view
   {
      const std::ptrdiff_t string_max_length = std::ssize(_data) - _head;
      char* const string_begin = reinterpret_cast<char*>(&_data[_head]);
      const auto string_end =
         std::find(string_begin, string_begin + string_max_length, '\0');

      const auto string_length = std::distance(string_begin, string_end);

      if (string_length == string_max_length || string_begin[string_length] != '\0') {
         throw std::runtime_error{
            "Bad string in chunk, was not null terminated."};
      }

      _head += string_length;

      check_head();

      if (!unaligned) align_head();

      return std::string_view{string_begin,
                              gsl::narrow_cast<std::size_t>(string_length)};
   }

   //! \brief Reads an unaligned null-terminated string from a chunk.
   //!
   //! \return A std::string_view to the string, excluding the null terminator.
   //!
   //! \exception std::runtime_error Thrown when reading the string would go
   //! past the end of the chunk.
   //! \exception std::runtime_error Thrown when reading the string would
   //! return a string that is not null-terminated.
   auto read_string_unaligned()
   {
      return read_string(true);
   }

   //! \brief Shifts the read head forward an amount of bytes.
   //!
   //! \param amount The amount to shift the head forward by.
   //! \param unaligned If the consume is unaligned or not.
   //!
   //! \exception std::runtime_error Thrown when the consume operation would go
   //! past the end of the chunk
   void consume(const std::ptrdiff_t amount, const bool unaligned = false)
   {
      _head += amount;

      check_head();

      if (!unaligned) align_head();
   }

   //! \brief Shifts the read head forward an unaligned amount of bytes.
   //!
   //! \param amount The amount to shift the head forward by.
   //!
   //! \exception std::runtime_error Thrown when the consume operation would go
   //! past the end of the chunk
   void consume_unaligned(const std::ptrdiff_t amount)
   {
      consume(amount, true);
   }

   //! \brief Tests if the end of the chunk has been reached or not.
   //!
   //! \return True if the end of the chunk has not been reached, false if it has.
   explicit operator bool() const noexcept
   {
      return (_head < _data.size());
   }

   //! \brief Reset the read head.
   void reset_head() noexcept
   {
      _head = 0;
   }

   //! \brief Gets the magic number of the chunk.
   //!
   //! \return The magic number of the chunk.
   Magic_number magic_number() const noexcept
   {
      return _mn;
   }

   //! \brief Gets size (in bytes) of the chunk.
   //!
   //! \return The size the chunk.
   std::ptrdiff_t size() const noexcept
   {
      return _data.size();
   }

private:
   void check_head()
   {
      if (_head > _data.size()) {
         throw std::runtime_error{"Attempt to read past end of chunk."};
      }
   }

   void align_head() noexcept
   {
      const auto remainder = _head % 4;

      if (remainder != 0) _head += (4 - remainder);
   }

   const Magic_number _mn;
   const std::span<std::byte> _data;

   std::span<std::byte>::size_type _head = 0;
};

//! \brief Find the first child chunk mathcing a magic number starting
//! from the beginning of a parent chunk.
//!
//! \param mn The magic number to look for.
//! \param from The Tweaker representing the parent chunk.
//!
//! \return The child chunk if found or std::nullopt in unfound.
inline auto find(Magic_number mn, Tweaker from) -> std::optional<Tweaker>
{
   from.reset_head();

   while (from) {
      auto child = from.get_child();

      if (child.magic_number() == mn) return child;
   }

   return std::nullopt;
}

//! \brief Find the next child chunk mathcing a magic number from a parent chunk.
//!
//! \param mn The magic number to look for.
//! \param from A rerence to the Tweaker representing the parent chunk.
//!
//! \return The child chunk if found or std::nullopt in unfound.
//!
//! Unlike find and find_all Tweaker::reset_head will not be called and
//! the search will take place from the current location of the chunk.
inline auto find_next(Magic_number mn, Tweaker& from) -> std::optional<Tweaker>
{
   while (from) {
      auto child = from.get_child();

      if (child.magic_number() == mn) return child;
   }

   return std::nullopt;
}

//! \brief Find all the child chunks mathcing a magic number starting
//! from the beginning of a parent chunk.
//!
//! \param mn The magic number to look for.
//! \param from The Tweaker representing the parent chunk.
//!
//! \return A SequenceContainer holding the found children, the container
//! will be empty if no matches are found.
inline auto find_all(Magic_number mn, Tweaker from) -> std::vector<Tweaker>
{
   from.reset_head();

   std::vector<Tweaker> matches;

   while (from) {
      auto child = from.get_child();

      if (child.magic_number() == mn) matches.emplace_back(child);
   }

   return matches;
}

}
