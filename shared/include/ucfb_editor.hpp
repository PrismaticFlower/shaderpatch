#pragma once

#include "magic_number.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_tweaker.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

#include <gsl/gsl>

namespace sp::ucfb {

class Writer;
class Editor_data_chunk;

class Editor_data_writer {
public:
   enum class Alignment : bool { unaligned, aligned };

   Editor_data_writer(Editor_data_chunk& data_chunk) noexcept;

   Editor_data_writer() = default;
   Editor_data_writer(const Editor_data_writer&) = delete;
   Editor_data_writer& operator=(const Editor_data_writer&) = delete;
   Editor_data_writer(Editor_data_writer&&) = delete;
   Editor_data_writer& operator=(Editor_data_writer&&) = delete;

   void write(const gsl::span<const std::byte> bytes,
              Alignment alignment = Alignment::aligned);

   template<typename Type>
   void write(const Type& value, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_standard_layout_v<Type>,
                    "Type must be standard layout!");
      static_assert(std::is_trivially_destructible_v<Type>,
                    "Type must be trivially destructible!");

      const auto bytes =
         gsl::make_span(reinterpret_cast<const std::byte*>(&value), sizeof(Type));

      write(bytes, alignment);
   }

   template<typename Type>
   void write(const gsl::span<Type> span, Alignment alignment = Alignment::aligned)
   {
      static_assert(std::is_standard_layout_v<Type>,
                    "Type must be standard layout!");
      static_assert(std::is_trivially_destructible_v<Type>,
                    "Type must be trivially destructible!");

      const auto bytes = gsl::as_bytes(span);

      write(bytes, alignment);
   }

   void write(const std::string_view string, Alignment alignment = Alignment::aligned);

   void write(const std::string& string, Alignment alignment = Alignment::aligned);

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
   void align() noexcept;

   Editor_data_chunk& _data;
};

class Editor_data_chunk : std::vector<std::byte> {
public:
   Editor_data_chunk() = default;

   explicit Editor_data_chunk(Reader reader) noexcept
   {
      const auto init = reader.read_array<std::byte>(reader.size());

      assign(init.begin(), init.end());
   }

   explicit Editor_data_chunk(const Editor_data_chunk&) = default;
   Editor_data_chunk& operator=(const Editor_data_chunk&) = delete;

   Editor_data_chunk(Editor_data_chunk&&) = default;
   Editor_data_chunk& operator=(Editor_data_chunk&&) = default;

   using value_type = std::byte;
   using size_type = std::uint32_t;
   using difference_type = std::int32_t;
   using reference = value_type&;
   using const_reference = const reference;
   using pointer = value_type*;
   using const_pointer = const value_type*;
   using iterator = std::vector<value_type>::iterator;
   using const_iterator = std::vector<value_type>::const_iterator;
   using reverse_iterator = std::vector<value_type>::reverse_iterator;
   using const_reverse_iterator = std::vector<value_type>::const_reverse_iterator;

   using std::vector<value_type>::begin;
   using std::vector<value_type>::end;
   using std::vector<value_type>::cbegin;
   using std::vector<value_type>::cend;
   using std::vector<value_type>::rbegin;
   using std::vector<value_type>::rend;

   using std::vector<value_type>::at;
   using std::vector<value_type>::operator[];
   using std::vector<value_type>::front;
   using std::vector<value_type>::back;

   using std::vector<value_type>::empty;
   using std::vector<value_type>::size;
   using std::vector<value_type>::max_size;
   using std::vector<value_type>::reserve;
   using std::vector<value_type>::capacity;
   using std::vector<value_type>::shrink_to_fit;

   using std::vector<value_type>::clear;
   using std::vector<value_type>::insert;
   using std::vector<value_type>::emplace;
   using std::vector<value_type>::erase;
   using std::vector<value_type>::push_back;
   using std::vector<value_type>::emplace_back;
   using std::vector<value_type>::pop_back;

   auto writer() noexcept -> Editor_data_writer
   {
      return *this;
   }

   auto span() noexcept -> gsl::span<value_type>
   {
      return gsl::make_span(data(), size());
   }

   auto span() const noexcept -> gsl::span<const value_type>
   {
      return gsl::make_span(data(), size());
   }
};

class Editor_parent_chunk
   : std::vector<std::pair<Magic_number, std::variant<Editor_data_chunk, Editor_parent_chunk>>> {
public:
   Editor_parent_chunk() = default;

   template<typename Filter>
   Editor_parent_chunk(Reader reader, Filter is_parent_chunk) noexcept
   {
      static_assert(std::is_nothrow_invocable_r_v<bool, Filter, Magic_number>, "is_parent_filter must be nothrow invocable, take a Magic_number of a chunk and return true or false depending on if the chunk is a parent or not.");

      while (reader) {
         const auto child = reader.read_child();

         if (is_parent_chunk(child.magic_number())) {
            emplace_back(child.magic_number(),
                         Editor_parent_chunk{child, is_parent_chunk});
         }
         else {
            emplace_back(child.magic_number(), Editor_data_chunk{child});
         }
      }
   }

   explicit Editor_parent_chunk(const Editor_parent_chunk&) = default;
   Editor_parent_chunk& operator=(const Editor_parent_chunk&) = delete;

   Editor_parent_chunk(Editor_parent_chunk&&) = default;
   Editor_parent_chunk& operator=(Editor_parent_chunk&&) = default;

   using value_type =
      std::pair<Magic_number, std::variant<Editor_data_chunk, Editor_parent_chunk>>;
   using size_type = std::uint32_t;
   using difference_type = std::int32_t;
   using reference = value_type&;
   using const_reference = const reference;
   using pointer = value_type*;
   using const_pointer = const value_type*;
   using iterator = std::vector<value_type>::iterator;
   using const_iterator = std::vector<value_type>::const_iterator;
   using reverse_iterator = std::vector<value_type>::reverse_iterator;
   using const_reverse_iterator = std::vector<value_type>::const_reverse_iterator;

   using std::vector<value_type>::begin;
   using std::vector<value_type>::end;
   using std::vector<value_type>::cbegin;
   using std::vector<value_type>::cend;
   using std::vector<value_type>::rbegin;
   using std::vector<value_type>::rend;

   using std::vector<value_type>::at;
   using std::vector<value_type>::operator[];
   using std::vector<value_type>::front;
   using std::vector<value_type>::back;

   using std::vector<value_type>::empty;
   using std::vector<value_type>::size;
   using std::vector<value_type>::max_size;
   using std::vector<value_type>::reserve;
   using std::vector<value_type>::capacity;
   using std::vector<value_type>::shrink_to_fit;

   using std::vector<value_type>::clear;
   using std::vector<value_type>::insert;
   using std::vector<value_type>::emplace;
   using std::vector<value_type>::erase;
   using std::vector<value_type>::push_back;
   using std::vector<value_type>::emplace_back;
   using std::vector<value_type>::pop_back;
};

class Editor : public Editor_parent_chunk {
public:
   Editor() noexcept = default;

   template<typename Filter>
   Editor(Reader_strict<"ucfb"_mn> reader, Filter&& is_parent_chunk) noexcept
      : Editor_parent_chunk{reader, std::forward<Filter>(is_parent_chunk)}
   {
   }

   void assemble(Writer& output) const noexcept;
};

inline auto make_reader(Editor_parent_chunk::const_iterator it) noexcept -> ucfb::Reader
{
   Expects(std::holds_alternative<Editor_data_chunk>(it->second));

   return ucfb::Reader{it->first, std::get<Editor_data_chunk>(it->second).span()};
}

template<Magic_number mn>
inline auto make_strict_reader(Editor_parent_chunk::const_iterator it) noexcept
   -> ucfb::Reader_strict<mn>
{
   Expects(std::holds_alternative<Editor_data_chunk>(it->second));

   return ucfb::Reader_strict<mn>{
      ucfb::Reader{it->first, std::get<Editor_data_chunk>(it->second).span()}};
}

template<typename It_first, typename It_last>
inline auto find(It_first first, It_last last, const Magic_number mn) noexcept -> It_first
{
   return std::find_if(first, last, [mn](const auto& v) { return v.first == mn; });
}

template<typename Editor>
inline auto find(Editor& editor, const Magic_number mn) noexcept
{
   return std::find_if(editor.begin(), editor.end(),
                       [mn](const auto& v) { return v.first == mn; });
}

template<typename Editor>
inline auto find_all(Editor& editor, const Magic_number mn) noexcept
{
   std::vector<decltype(editor.begin())> results;
   results.reserve(16);

   for (auto it = find(editor, mn); it != editor.end();
        it = ucfb::find(it + 1, editor.end(), mn)) {
      results.emplace_back(it);
   }

   return results;
}

}
