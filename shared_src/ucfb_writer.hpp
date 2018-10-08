#pragma once

#include <cstdint>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <gsl/gsl>

#include "magic_number.hpp"

namespace sp::ucfb {

template<typename Last_act, typename Writer_type>
class Writer_child : public Writer_type {
   static_assert(std::is_invocable_v<Last_act, decltype(Writer_type::_size)>);

public:
   Writer_child(Last_act last_act, std::ostream& output_stream, const Magic_number mn)
      : Writer_type{output_stream, mn}, _last_act{last_act}
   {
   }

   ~Writer_child()
   {
      _last_act(Writer_type::_size);
   }

private:
   Last_act _last_act;
};

class Writer {
public:
   enum class Alignment : bool { unaligned, aligned };

   Writer(std::ostream& output_stream, const Magic_number root_mn = "ucfb"_mn)
      : _out{output_stream}
   {
      Expects(output_stream.good());

      _out.write(reinterpret_cast<const char*>(&root_mn), sizeof(Magic_number));

      _size_pos = _out.tellp();
      _out.write(reinterpret_cast<const char*>(&size_place_hold),
                 sizeof(size_place_hold));
   }

   ~Writer()
   {
      const auto cur_pos = _out.tellp();

      const auto chunk_size = static_cast<std::uint32_t>(_size);

      _out.seekp(_size_pos);
      _out.write(reinterpret_cast<const char*>(&chunk_size), sizeof(chunk_size));
      _out.seekp(cur_pos);
   }

   Writer() = delete;

   Writer& operator=(const Writer&) = delete;
   Writer& operator=(Writer&&) = delete;

   auto emplace_child(const Magic_number mn)
   {
      const auto last_act = [this](auto child_size) { _size += child_size; };

      align_file();

      _size += 8;

      return Writer_child<decltype(last_act), Writer>{last_act, _out, mn};
   }

   template<typename Type>
   void write(const Type& value, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable!");

      _out.write(reinterpret_cast<const char*>(&value), sizeof(Type));
      _size += sizeof(Type);

      if (alignment == Alignment::aligned) align_file();
   }

   template<typename Type>
   void write(const gsl::span<Type> span, Alignment alignment = Alignment::aligned)
   {
      _out.write(reinterpret_cast<const char*>(span.data()), span.size_bytes());
      _size += span.size_bytes();

      if (alignment == Alignment::aligned) align_file();
   }

   void write(const std::string_view string, Alignment alignment = Alignment::aligned)
   {
      _out.write(string.data(), string.size());
      _out.put('\0');

      _size += string.length() + 1;

      if (alignment == Alignment::aligned) align_file();
   }

   void write(const std::string& string, Alignment alignment = Alignment::aligned)
   {
      write(std::string_view{string}, alignment);
   }

   template<typename... Args>
   auto write(Args&&... args)
      -> std::enable_if_t<!std::disjunction_v<std::is_same<Alignment, Args>...>>
   {
      (this->write(args, Alignment::aligned), ...);
   }

   template<typename... Args>
   auto write_unaligned(Args&&... args)
      -> std::enable_if_t<!std::disjunction_v<std::is_same<Alignment, Args>...>>
   {
      (this->write(args, Alignment::unaligned), ...);
   }

private:
   template<typename Last_act, typename Writer_type>
   friend class Writer_child;

   constexpr static std::uint32_t size_place_hold = 0;

   Writer(const Writer&) = default;
   Writer(Writer&&) = default;

   void align_file() noexcept
   {
      const auto remainder = _size % 4;

      if (remainder != 0) {
         _out.write("\0\0\0\0", (4 - remainder));

         _size += (4 - remainder);
      }
   }

   std::ostream& _out;
   std::streampos _size_pos;
   std::int64_t _size{};
};

inline auto open_file_for_output(const std::string& file_name) -> std::ofstream
{
   using namespace std::literals;

   std::ofstream file;

   file.open(file_name, std::ios::binary);

   if (!file.is_open()) {
      throw std::runtime_error{"Unable to open file \""s + file_name + "\" for output."s};
   }

   return file;
}
}
