#pragma once

#include "compose_exception.hpp"
#include "magic_number.hpp"
#include "small_function.hpp"
#include "utility.hpp"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <gsl/gsl>

namespace sp::ucfb {

// clang-format off
template<typename T>
concept Writer_target_position = requires(const T& target) {
   { target.position() } -> std::copyable;
   { target.position() } -> std::convertible_to<std::uint32_t>;
};

template<typename T>
concept Writer_target_output = requires(T& target, std::span<const std::byte> out_data, 
                                        std::invoke_result_t<decltype(&T::position), T> position) {
   { target.write(out_data) };
   { target.write_at_position(out_data, position) };
};

template<typename T>
concept Writer_target = Writer_target_position<T> && Writer_target_output<T>;
// clang-format on

class Writer_target_ostream {
public:
   explicit Writer_target_ostream(std::ostream& out) : _out{out}
   {
      Expects(out.good());
   }

   auto position() const -> std::streampos
   {
      return _out.tellp();
   }

   void write(std::span<const std::byte> out_data)
   {
      _out.write(reinterpret_cast<const char*>(out_data.data()), out_data.size());
   }

   void write_at_position(std::span<const std::byte> out_data, std::streampos position)
   {
      const auto cur_pos = _out.tellp();

      _out.seekp(position);

      write(out_data);

      _out.seekp(cur_pos);
   }

private:
   std::ostream& _out;
};

template<typename T>
class Writer_target_container {
public:
   explicit Writer_target_container(T& out) : _out{out} {}

   auto position() const noexcept -> std::size_t
   {
      return _out.size();
   }

   void write(std::span<const std::byte> out_data)
   {
      _out.insert(_out.end(), out_data.begin(), out_data.end());
   }

   void write_at_position(std::span<const std::byte> out_data, std::size_t position)
   {
      if (position + out_data.size() > _out.size()) {
         _out.resize(position + out_data.size());
      }

      auto where = _out.begin();

      std::ranges::advance(where, position);
      std::ranges::copy(out_data, where);
   }

private:
   T& _out;
};

template<Writer_target Output, typename Last_act, typename Writer_type>
class Writer_child : public Writer_type {
   static_assert(std::is_invocable_v<Last_act, decltype(Writer_type::_size)>);

public:
   Writer_child(Output& output, const Magic_number mn, Last_act last_act)
      : Writer_type{mn, output}, _last_act{std::move(last_act)}
   {
   }

   Writer_child(const Writer_child&) = delete;
   Writer_child& operator=(const Writer_child&) = delete;

   Writer_child(Writer_child&&) = delete;
   Writer_child& operator=(Writer_child&&) = delete;

   ~Writer_child()
   {
      _last_act(Writer_type::_size);
   }

private:
   Last_act _last_act;
};

template<Writer_target Output>
class Writer {
public:
   enum class Alignment : bool { aligned, unaligned };

   template<typename... Output_args>
   Writer(const Magic_number root_mn,
          Output_args&&... output) requires std::constructible_from<Output, Output_args...>
      : _out{std::forward<Output_args>(output)...}
   {
      _out.write(std::as_bytes(std::span{&root_mn, 1}));

      _size_pos = _out.position();
      _out.write(std::as_bytes(std::span{&size_place_hold, 1}));
   }

   ~Writer()
   {
      const auto chunk_size = static_cast<std::int32_t>(_size);

      _out.write_at_position(std::as_bytes(std::span{&chunk_size, 1}), _size_pos);
   }

   Writer() = delete;

   Writer(const Writer&) = delete;
   Writer& operator=(const Writer&) = delete;

   Writer(Writer&&) = delete;
   Writer& operator=(Writer&&) = delete;

   auto emplace_child(const Magic_number mn)
   {
      const auto last_act = [this](auto child_size) {
         increase_size(child_size);
      };

      align_file();

      increase_size(8);

      return Writer_child<Output, decltype(last_act), Writer>{_out, mn, last_act};
   }

   void write(const std::span<const std::byte> span,
              Alignment alignment = Alignment::aligned)
   {
      _out.write(span);
      increase_size(span.size());

      if (alignment == Alignment::aligned) align_file();
   }

   template<typename Type>
   void write(const std::span<Type> span, Alignment alignment = Alignment::aligned)
   {
      write(std::as_bytes(span), alignment);
   }

   template<typename Type>
   void write(const Type& value, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_standard_layout_v<Type>,
                    "Type must be standard layout!");
      static_assert(std::is_trivially_destructible_v<Type>,
                    "Type must be trivially destructible!");
      static_assert(!std::is_same_v<std::remove_cvref_t<Type>, Alignment>,
                    "Type must not be the Alignment type!");

      write(std::span{&value, 1}, alignment);
   }

   void write(const std::string_view string, Alignment alignment = Alignment::aligned)
   {
      write(std::span{string}, Alignment::unaligned);
      write('\0', alignment);
   }

   void write(const std::string& string, Alignment alignment = Alignment::aligned)
   {
      write(std::string_view{string}, alignment);
   }

   template<typename... Args>
   auto write(const Args&... args)
      -> std::enable_if_t<!std::disjunction_v<std::is_same<Alignment, Args>...>>
   {
      (this->write(args, Alignment::aligned), ...);
   }

   template<typename... Args>
   auto write_unaligned(const Args&... args)
      -> std::enable_if_t<!std::disjunction_v<std::is_same<Alignment, Args>...>>
   {
      (this->write(args, Alignment::unaligned), ...);
   }

   auto pad(const std::uint32_t amount, Alignment alignment = Alignment::aligned)
   {
      for (auto i = 0u; i < amount; ++i) {
         write(std::array{std::byte{}}, Alignment::unaligned);
      }

      if (alignment == Alignment::aligned) align_file();
   }

   auto pad_unaligned(const std::uint32_t amount)
   {
      return pad(amount, Alignment::unaligned);
   }

   auto absolute_size() const noexcept -> std::uint32_t
   {
      return gsl::narrow_cast<std::uint32_t>(_out.position());
   }

private:
   template<Writer_target Output, typename Last_act, typename Writer_type>
   friend class Writer_child;

   using Position = std::invoke_result_t<decltype(&Output::position), Output>;

   constexpr static std::uint32_t size_place_hold = 0;
   constexpr static std::int64_t write_alignment = 4;

   void increase_size(const std::int64_t len)
   {
      _size += len;

      if (_size > std::numeric_limits<std::int32_t>::max()) {
         throw std::runtime_error{"ucfb file too large!"};
      }

      Ensures(_size <= std::numeric_limits<std::int32_t>::max());
   }

   void align_file()
   {
      constexpr std::array<const std::byte, 4> data{};

      const auto alignment_bytes =
         static_cast<std::size_t>(next_multiple_of<write_alignment>(_size) - _size);

      if (alignment_bytes == 0) return;

      write(std::span{data}.subspan(0, alignment_bytes), Alignment::unaligned);
   }

   Output _out;
   Position _size_pos;
   std::int64_t _size{};
};

using File_writer = Writer<Writer_target_ostream>;

template<typename T>
struct Memory_writer : Writer<Writer_target_container<T>> {
   using Writer<Writer_target_container<T>>::Writer;
};

template<typename T>
Memory_writer(const Magic_number, T&) -> Memory_writer<T>;

//! \brief Helping for writing _from_ an alignment in a chunk.
//!
//! \tparam alignment The alignment to write till.
//!
//! \param writer The writer to write the data to.
//! \param data The data to write.
//!
//! This function does writes two things, first at the current position in writer it writes
//! the offset (in a uint32) from _after_ it that the data will be written. Then it write the data.
template<std::size_t alignment, Writer_target Output>
inline void write_at_alignment(Writer<Output>& writer,
                               const std::span<const std::byte> data) noexcept
{
   // Calculate needed alignment.
   const auto align_from_size = writer.absolute_size() + sizeof(std::uint32_t);
   const auto aligned_offset =
      next_multiple_of<alignment>(align_from_size) - align_from_size;

   // Write alignment offset.
   writer.write(gsl::narrow_cast<std::uint32_t>(aligned_offset));

   // Pad until aligned.
   writer.pad_unaligned(gsl::narrow_cast<std::uint32_t>(aligned_offset));

   // Write data.
   writer.write(data);
}

inline auto open_file_for_output(const std::filesystem::path& file_path) -> std::ofstream
{
   using namespace std::literals;

   std::ofstream file;

   file.open(file_path, std::ios::binary);

   if (!file.is_open()) {
      throw compose_exception<std::runtime_error>("Unable to open file "sv,
                                                  file_path, " for output."sv);
   }

   return file;
}
}
