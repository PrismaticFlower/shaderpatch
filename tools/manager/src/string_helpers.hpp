#pragma once

#include <array>
#include <string_view>

inline auto split_first_of_exclusive(std::string_view str, std::string_view delimiter) noexcept
   -> std::array<std::string_view, 2>
{
   using namespace std::literals;

   const auto offset = str.find(delimiter);

   if (offset == str.npos) return {str, ""sv};

   return {str.substr(0, offset),
           str.substr(offset + delimiter.size(), str.size() - offset)};
}

class lines_iterator {
public:
   using value_type = std::string_view;
   using pointer = std::string_view*;
   using reference = std::string_view&;
   using iterator_category = std::input_iterator_tag;

   explicit lines_iterator(std::string_view str) noexcept : _str{str}
   {
      advance();
   }

   lines_iterator() : _is_end{true} {}

   auto operator++() noexcept -> lines_iterator&
   {
      advance();

      return *this;
   }

   void operator++(int) noexcept
   {
      advance();
   }

   auto operator*() const noexcept -> const std::string_view&
   {
      return _line;
   }

   auto operator->() const noexcept -> const std::string_view&
   {
      return _line;
   }

   auto begin() noexcept -> lines_iterator
   {
      return *this;
   }

   auto end() noexcept -> lines_iterator
   {
      return {};
   }

   bool operator==(const lines_iterator&) const noexcept
   {
      return _is_end;
   }

   bool operator!=(const lines_iterator&) const noexcept
   {
      return !_is_end;
   }

private:
   void advance() noexcept
   {
      using namespace std::literals;

      if (_next_is_end) {
         _is_end = true;

         return;
      }

      auto [line, rest] = split_first_of_exclusive(_str, "\n"sv);
      _str = rest;

      if (not line.empty() and line.back() == '\r') {
         line = line.substr(0, line.size() - 1);
      }

      _line = line;

      _next_is_end = _str.empty();
   }

   std::string_view _str;
   std::string_view _line;
   bool _next_is_end = false;
   bool _is_end = false;
};

/// @brief A very basic token iterator. It considers only ' ' to be whitespace.
class token_iterator {
public:
   using value_type = std::string_view;
   using pointer = std::string_view*;
   using reference = std::string_view&;
   using iterator_category = std::input_iterator_tag;

   explicit token_iterator(std::string_view str) noexcept : _str{str}
   {
      advance();
   }

   token_iterator() : _is_end{true} {}

   auto operator++() noexcept -> token_iterator&
   {
      advance();

      return *this;
   }

   void operator++(int) noexcept
   {
      advance();
   }

   auto operator*() const noexcept -> const std::string_view&
   {
      return _token;
   }

   auto operator->() const noexcept -> const std::string_view&
   {
      return _token;
   }

   auto begin() noexcept -> token_iterator
   {
      return *this;
   }

   auto end() noexcept -> token_iterator
   {
      return {};
   }

   bool operator==(const token_iterator&) const noexcept
   {
      return _is_end;
   }

   bool operator!=(const token_iterator&) const noexcept
   {
      return !_is_end;
   }

private:
   void advance() noexcept
   {
      using namespace std::literals;

      if (_next_is_end) {
         _is_end = true;

         return;
      }

      auto [token, rest] = split_first_of_exclusive(_str, " "sv);
      _str = rest;
      _token = token;

      _next_is_end = _str.empty();
   }

   std::string_view _str;
   std::string_view _token;
   bool _next_is_end = false;
   bool _is_end = false;
};
