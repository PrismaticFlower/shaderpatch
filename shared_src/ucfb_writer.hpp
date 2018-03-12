#pragma once

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/smart_ptr/local_shared_ptr.hpp>
#include <gsl/gsl>

#include "magic_number.hpp"

namespace sp {

template<typename Last_act>
class Ucfb_writer_child : public Ucfb_writer {
   static_assert(std::is_invocable_v<Last_act, decltype(_size)>);

public:
   Ucfb_writer_child(Last_act last_act, boost::local_shared_ptr<std::ofstream> file,
                     const Magic_number mn)
      : Ucfb_writer{std::move(file), mn}, _last_act{last_act}
   {
   }

   ~Ucfb_writer_child()
   {
      _last_act(_size);
   }

private:
   Last_act _last_act;
};

class Ucfb_writer {
public:
   enum class Alignment : bool { unaligned, aligned };

   Ucfb_writer(const std::string_view file)
   {
      using namespace std::literals;

      _file = std::make_unique<std::ofstream>(std::string{file}, std::ios::binary);

      if (!_file->is_open()) {
         throw std::runtime_error{"Unable to open file \""s +
                                  std::string{file} + "\" for output."s};
      }

      constexpr auto mn = "ucfb"_mn;

      _file->write(reinterpret_cast<const char*>(&mn), sizeof(Magic_number));

      _size_pos = _file->tellp();
      _file->write(reinterpret_cast<const char*>(&size_place_hold),
                   sizeof(size_place_hold));
   }

   ~Ucfb_writer()
   {
      const auto cur_pos = _file->tellp();

      const auto chunk_size = static_cast<std::uint32_t>(_size);

      _file->seekp(_size_pos);
      _file->write(reinterpret_cast<const char*>(&chunk_size), sizeof(chunk_size));
      _file->seekp(cur_pos);
   }

   Ucfb_writer() = delete;

   Ucfb_writer& operator=(const Ucfb_writer&) = delete;
   Ucfb_writer& operator=(Ucfb_writer&&) = delete;

   auto emplace_child(const Magic_number mn)
   {
      const auto last_act = [this](auto child_size) { _size += child_size; };

      align_file();

      _size += 8;

      return Ucfb_writer_child<decltype(last_act)>{last_act, _file, mn};
   }

   template<typename Type>
   void write(const Type& value, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable!");

      _file->write(reinterpret_cast<const char*>(&value), sizeof(Type));
      _size += sizeof(Type);

      if (alignment == Alignment::aligned) align_file();
   }

   template<typename Type>
   void write(const gsl::span<Type> span, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_trivially_copyable_v<Type>,
                    "Type must be trivially copyable!");

      _file->write(reinterpret_cast<const char*>(span.data()), span.size_bytes());
      _size += span.size_bytes();

      if (alignment == Alignment::aligned) align_file();
   }

   void write(const std::string_view string, Alignment alignment = Alignment::aligned)
   {
      *_file << string;
      _file->put('\0');

      _size += string.length() + 1;

      if (alignment == Alignment::aligned) align_file();
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
   template<typename Last_act>
   friend class Ucfb_writer_child;

   constexpr static std::uint32_t size_place_hold = 0;

   Ucfb_writer(const Ucfb_writer&) = default;
   Ucfb_writer(Ucfb_writer&&) = default;

   Ucfb_writer(boost::local_shared_ptr<std::ofstream> file, const Magic_number mn) noexcept
      : _file{file}
   {
      _file->write(reinterpret_cast<const char*>(&mn), sizeof(Magic_number));

      _size_pos = _file->tellp();
      _file->write(reinterpret_cast<const char*>(&size_place_hold),
                   sizeof(size_place_hold));
   }

   void align_file() noexcept
   {
      const auto remainder = _size % 4;

      if (remainder != 0) {
         _file->write("\0\0\0\0", (4 - remainder));

         _size += (4 - remainder);
      }
   }

   boost::local_shared_ptr<std::ofstream> _file;
   std::streampos _size_pos;
   std::int64_t _size{};
};
}
