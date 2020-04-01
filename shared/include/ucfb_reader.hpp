#pragma once

#include "magic_number.hpp"
#include "utility.hpp"

#include <gsl/gsl>

#include <cstddef>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

namespace sp::ucfb {

template<Magic_number type_mn>
class Reader_strict;

//! \brief The class used for reading SWBF's game files. Modified version of the
//! class used in swbf-unmunge.
//!
//! Each reader represents a non-owning view at a chunk from a "ucfb" file.
//! A reader holds only one piece of mutable state and that is the offset to the
//! next unread byte. When a reader is copied, this offset is also copied. It
//! can however be reset at anytime by calling Reader::reset_head.
//!
//! The various `read_*` all advance the read head by the size of the value
//! they read. The read can be aligned or unaligned. When it is aligned the
//! read head is advanced to the next four byte margin after the read, else
//! it is only advanced by the size of the value.
//!
//! An inidividual instance of this class is not threadsafe, however because it
//! only represents an immutable view the multiple threads can safely hold
//! different readers all reading the same chunk.
class Reader {
public:
   Reader() = delete;

   //! \brief Creates a Reader from a span of memory.
   //!
   //! \param bytes The span of memory holding the ucfb chunk. The size of the
   //! span must be at least 8.
   //!
   //! \exception std::runtime_error Thrown when the size of the chunk does not
   //! match the size of the span.
   Reader(const gsl::span<const std::byte> bytes)
      : _mn{bit_cast<Magic_number>(bytes)},
        _size{bit_cast<std::uint32_t>(
           bytes.subspan(sizeof(Magic_number), sizeof(std::uint32_t)))},
        _data{&bytes[8]}
   {
      Expects((bytes.size() >= 8));

      if (_size > static_cast<std::size_t>(bytes.size() - 8)) {
         throw std::runtime_error{
            "Size of supplied memory is less than size of supposed chunk."};
      }
   }

   //! \brief Creates a Reader from a magic number and a span of memory.
   //! The span of memory excludes the magic number and size tag of the chunk.
   //!
   //! \param mn The magic number of the reader.
   //! \param bytes The span of memory holding the ucfb chunk. The size of the
   //! span must be at least 8.
   //!
   Reader(const Magic_number mn, const gsl::span<const std::byte> bytes)
      : _mn{mn}, _size{static_cast<std::size_t>(bytes.size())}, _data{bytes.data()}
   {
   }

   //! \brief Reads a trivial value from the chunk.
   //!
   //! \tparam Type The type of the value to read. The type must be standard layout.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return The read value.
   //!
   //! \exception std::runtime_error Thrown when reading the value would go past the end
   //! of the chunk.
   template<typename Type>
   auto read(const bool unaligned = false) -> Type
   {
      static_assert(std::is_standard_layout_v<Type>,
                    "Type must be standard layout.");
      static_assert(!std::is_reference_v<Type>, "Type can not be a reference.");
      static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

      const auto cur_pos = _head;
      _head += sizeof(Type);

      check_head();

      if (!unaligned) align_head();

      return bit_cast<Type>(gsl::make_span(&_data[cur_pos], sizeof(Type)));
   }

   //! \brief Reads a trivial unaligned value from the chunk.
   //!
   //! \tparam Type The type of the value to read. The type must be standard layout.
   //!
   //! \return The read value.
   //!
   //! \exception std::runtime_error Thrown when reading the value would go past the end
   //! of the chunk.
   template<typename Type>
   auto read_unaligned() -> Type
   {
      return read<Type>(true);
   }

   //! \brief Reads a list of trivial value from the chunk.
   //!
   //! \tparam Types A list of types of the values to read. The types must be standard layout.
   //!
   //! \return A tuple of the values.
   //!
   //! \exception std::runtime_error Thrown when reading the value would go past the end
   //! of the chunk.
   template<typename... Types>
   auto read_multi(const std::array<bool, sizeof...(Types)> unaligned = {})
      -> std::tuple<Types...>
   {
      return read_multi_impl<Types...>(unaligned, std::index_sequence_for<Types...>{});
   }

   //! \brief Reads a list of trivial unaligned value from the chunk.
   //!
   //! \tparam Types A list of types of the values to read. The types must be standard layout.
   //!
   //! \return A tuple of the values.
   //!
   //! \exception std::runtime_error Thrown when reading the value would go past the end
   //! of the chunk.
   template<typename... Types>
   auto read_multi_unaligned() -> std::tuple<Types...>
   {
      return read_multi<Types...>(std::array{(sizeof(Types), true)...});
   }

   //! \brief Reads a variable-length array of trivial values from the chunk.
   //!
   //! \tparam Type The type of the values to read. The type must be trivially copyable.
   //!
   //! \param size The size of the array to read.
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A span of const Type.
   //!
   //! \exception std::runtime_error Thrown when reading the array would go past the end
   //! of the chunk.
   template<typename Type>
   auto read_array(const std::size_t size, const bool unaligned = false)
      -> gsl::span<const Type>
   {
      static_assert(std::is_standard_layout_v<Type>,
                    "Type must be standard layout.");
      static_assert(!std::is_reference_v<Type>, "Type can not be a reference.");
      static_assert(!std::is_pointer_v<Type>, "Type can not be a pointer.");

      const auto cur_pos = _head;
      _head += sizeof(Type) * size;

      check_head();

      if (!unaligned) align_head();

      return {reinterpret_cast<const Type*>(&_data[cur_pos]), size};
   }

   //! \brief Reads an unaligned variable-length array of trivial values from the chunk.
   //!
   //! \tparam Type The type of the values to read. The type must be trivially copyable.
   //!
   //! \param size The size of the array to read.
   //!
   //! \return A span of const Type.
   //!
   //! \exception std::runtime_error Thrown when reading the array would go past the end
   //! of the chunk.
   template<typename Type>
   auto read_array_unaligned(const std::size_t size) -> gsl::span<const Type>
   {
      return read_array<Type>(size, true);
   }

   //! \brief Reads a null-terminated string from a chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A string_view to the string.
   //!
   //! \exception std::runtime_error Thrown when reading the string would go
   //! past the end of the chunk.
   auto read_string(const bool unaligned = false) -> std::string_view
   {
      const char* const string = reinterpret_cast<const char*>(_data + _head);
      const auto string_length = cstring_length(string, _size - _head);

      _head += (string_length + 1);

      check_head();

      if (!unaligned) align_head();

      return {string, string_length};
   }

   //! \brief Reads an unaligned null-terminated string from a chunk.
   //!
   //! \return A string_view to the string.
   //!
   //! \exception std::runtime_error Thrown when reading the string would go
   //! past the end of the chunk.
   auto read_string_unaligned() -> std::string_view
   {
      return read_string(true);
   }

   //! \brief Reads a child chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A new Reader for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when reading the child would go past
   //! the end of the current chunk.
   Reader read_child(const bool unaligned = false)
   {
      const auto child_mn = read<Magic_number>();
      const auto child_size = read<std::uint32_t>();
      const auto child_data_offset = _head;

      _head += child_size;

      check_head();

      if (!unaligned) align_head();

      return Reader{child_mn, child_size, _data + child_data_offset};
   }

   //! \brief Reads an unaligned child chunk.
   //!
   //! \return A new Reader for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when reading the child would go past
   //! the end of the current chunk.
   Reader read_child_unaligned()
   {
      return read_child(true);
   }

   //! \brief Attempts to read a child chunk without the possibility of throwing
   //! an exception.
   //!
   //! \param <unnamed> std tag type for specifying the noexcept function.
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A std::optional<Reader> for the child chunk. If the read
   //! failed (because it would overflow the chunk) nullopt is returned instead.
   auto read_child(const std::nothrow_t, const bool unaligned = false) noexcept
      -> std::optional<Reader>
   {
      if ((_head + 8) > _size) return std::nullopt;

      const auto old_head = _head;

      const auto child_mn = read<Magic_number>();
      const auto child_size = read<std::uint32_t>();
      const auto child_data_offset = _head;

      _head += child_size;

      if (_head > _size) {
         _head = old_head;

         return std::nullopt;
      }

      if (!unaligned) align_head();

      return Reader{child_mn, child_size, _data + child_data_offset};
   }

   //! \brief Attempts to read an unaligned child chunk without the possibility
   //! of throwing an exception.
   //!
   //! \param <unnamed> std tag type for specifying the noexcept function.
   //!
   //! \return A std::optional<Reader> for the child chunk. If the read
   //! failed (because it would overflow the chunk) nullopt is returned instead.
   auto read_child_unaligned(const std::nothrow_t) noexcept -> std::optional<Reader>
   {
      return read_child(std::nothrow, true);
   }

   //! \brief Reads a child if it's magic number matches an expected
   //! magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A new Reader_strict<type_mn> for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when the magic number of the child
   //! and the expected magic number do not match. If this happens the read head
   //! is not moved. \exception std::runtime_error Thrown when reading the child
   //! would go past the end of the current chunk.
   template<Magic_number type_mn>
   auto read_child_strict(const bool unaligned = false) -> Reader_strict<type_mn>
   {
      return {read_child_strict(type_mn, unaligned),
              typename Reader_strict<type_mn>::Unchecked_tag{}};
   }

   //! \brief Reads an unaligned child if it's magic number matches an expected
   //! magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \return A new Reader_strict<type_mn> for the child chunk.
   //!
   //! \exception std::runtime_error Thrown when the magic number of the child
   //! and the expected magic number do not match. If this happens the read head
   //! is not moved. \exception std::runtime_error Thrown when reading the child
   //! would go past the end of the current chunk.
   template<Magic_number type_mn>
   auto read_child_strict_unaligned() -> Reader_strict<type_mn>
   {
      return read_child_strict<type_mn>(true);
   }

   //! \brief Reads a child if it's magic number matches an expected
   //! magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \param unaligned If the read is unaligned or not.
   //!
   //! \return A std::optional<Reader_strict<type_mn>> for the child chunk.
   //! If the magic number does not match std::nullopt is returned.
   //!
   //! \exception std::runtime_error Thrown when reading the child would go past
   //! the end of the current chunk.
   template<Magic_number type_mn>
   auto read_child_strict_optional(const bool unaligned = false)
      -> std::optional<Reader_strict<type_mn>>
   {
      const auto child = read_child_strict_optional(type_mn, unaligned);

      if (child) {
         return {{*child, typename Reader_strict<type_mn>::Unchecked_tag{}}};
      }

      return {};
   }

   //! \brief Reads an unaligned child if it's magic number matches an expected
   //! magic number.
   //!
   //! \tparam type_mn The expected Magic_number of the child chunk.
   //!
   //! \return A std::optional<Reader_strict<type_mn>> for the child chunk.
   //! If the magic number does not match std::nullopt is returned.
   //!
   //! \exception std::runtime_error Thrown when reading the child would go past
   //! the end of the current chunk.
   template<Magic_number type_mn>
   auto read_child_strict_optional_unaligned()
      -> std::optional<Reader_strict<type_mn>>
   {
      return read_child_strict_optional<type_mn>(true);
   }

   //! \brief Shifts the read head forward an amount of bytes.
   //!
   //! \param amount The amount to shift the head forward by.
   //! \param unaligned If the consume is unaligned or not.
   //!
   //! \exception std::runtime_error Thrown when the consume operation would go
   //! past the end of the chunk
   void consume(const std::size_t amount, const bool unaligned = false)
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
   void consume_unaligned(const std::size_t amount)
   {
      consume(amount, true);
   }

   //! \brief Tests if the end of the chunk has been reached or not.
   //!
   //! \return True if the end of the chunk has not been reached, false if it has.
   explicit operator bool() const noexcept
   {
      return (_head < _size);
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
   std::size_t size() const noexcept
   {
      return _size;
   }

private:
   // Special constructor for use by read_child, performs no error checking.
   Reader(const Magic_number mn, const std::uint32_t size, const std::byte* const data)
      : _mn{mn}, _size{size}, _data{data}
   {
   }

   Reader read_child_strict(const Magic_number child_mn, const bool unaligned)
   {
      const auto old_head = _head;

      const auto child = read_child(unaligned);

      if (child.magic_number() != child_mn) {
         _head = old_head;

         throw std::runtime_error{
            "Chunk magic number mistmatch"
            " when performing strict read of child chunk."};
      }

      return child;
   }

   auto read_child_strict_optional(const Magic_number child_mn, const bool unaligned)
      -> std::optional<Reader>
   {
      const auto old_head = _head;

      const auto child = read_child(unaligned);

      if (child.magic_number() != child_mn) {
         _head = old_head;

         return {};
      }

      return child;
   }

   template<typename... Types, std::size_t... indices>
   auto read_multi_impl(const std::array<bool, sizeof...(Types)> unaligned,
                        std::index_sequence<indices...>) -> std::tuple<Types...>
   {
      return {read<Types>(unaligned[indices])...};
   }

   void check_head()
   {
      if (_head > _size) {
         throw std::runtime_error{"Attempt to read past end of chunk."};
      }
   }

   void align_head() noexcept
   {
      const auto remainder = _head % 4;

      if (remainder != 0) _head += (4 - remainder);
   }

   template<typename Char_type, typename Size_type>
   static Size_type cstring_length(const Char_type* const string, const Size_type max_length)
   {
      const auto string_end =
         std::find(string, string + max_length, static_cast<Char_type>('\0'));

      return static_cast<Size_type>(std::distance(string, string_end));
   }

   const Magic_number _mn;
   const std::size_t _size;
   const std::byte* const _data;

   std::size_t _head = 0;
};

//! \brief A class used to restrict a reader to a specific magic number.
//!
//! \tparam type_mn The magic number to restrict the reader to.
template<Magic_number type_mn>
class Reader_strict : public Reader {
public:
   Reader_strict() = delete;

   //! \brief Construct a strict reader from a span of memory.
   //!
   //! \param bytes The span of memory holding the chunk. The size of the
   //! span must be at least 8 and the first four bytes must match `type_mn`.
   //!
   //! \exception std::runtime_error Thrown when the size of the chunk does not
   //! match the size of the span.
   explicit Reader_strict(const gsl::span<const std::byte> bytes)
      : Reader{bytes}
   {
      Expects(magic_number() == type_mn);
   }

   //! \brief Construct a strict reader.
   //!
   //! \param ucfb_reader The reader to construct the strict reader from.
   //! The magic number of the reader must match type_mn.
   explicit Reader_strict(Reader ucfb_reader) noexcept : Reader{ucfb_reader}
   {
      Expects(type_mn == ucfb_reader.magic_number());
   }

private:
   friend class Reader;

   struct Unchecked_tag {
   };

   Reader_strict(Reader ucfb_reader, Unchecked_tag) : Reader{ucfb_reader} {}
};

//! \brief Read children from a chunk until one with the specified magic number is found.
//!
//! \tparam mn The magic number to search for.
//!
//! \param reader A reference to the Reader to search through.
//!
//! \return A Reader_strict of the first child found with the specified magic number.
//!
//! \exception std::runtime_error Thrown when a child with  can not be found.
template<Magic_number mn>
inline auto skip_to_child(Reader& reader) -> Reader_strict<mn>
{
   while (reader) {
      auto child = reader.read_child();

      if (child.magic_number() == mn) return Reader_strict<mn>{child};
   }

   throw std::runtime_error{
      "couldn't find child with expected magic number in ucfb reader"};
}

}
