#pragma once

#include <array>
#include <cctype>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>

#include <gsl/gsl>

namespace sp {

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   const auto offset = string.find_first_of(delimiter);

   if (offset == string.npos) return {string, std::string_view{""}};

   std::array<std::basic_string_view<Char_t, Char_triats>, 2> split_strings;

   split_strings[0] = string.substr(0, offset);
   split_strings[1] = string.substr(offset + delimiter.length(),
                                    string.length() - offset - delimiter.length());

   return split_strings;
}

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string(
   const Char_t* string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return split_string(std::basic_string_view<Char_t, Char_triats>{string}, delimiter);
}

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
inline auto split_string(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return split_string(std::basic_string_view<Char_t, Char_triats>{string}, delimiter);
}

struct Ci_char_traits : public std::char_traits<char> {
   static bool eq(char l, char r)
   {
      return std::tolower(l) == std::tolower(r);
   }

   static bool lt(char l, char r)
   {
      return std::tolower(l) < std::tolower(r);
   }

   static int compare(const char* l, const char* r, std::size_t size)
   {
      return _memicmp(l, r, size);
   }

   static const char* find(const char* p, std::size_t count, const char& ch)
   {
      const auto lower_ch = std::tolower(ch);

      for (const auto& c : gsl::make_span(p, count)) {
         if (std::tolower(c) == lower_ch) {
            return &c;
         }
      }

      return nullptr;
   }
};

using Ci_string = std::basic_string<char, Ci_char_traits>;
using Ci_String_view = std::basic_string_view<char, Ci_char_traits>;

inline Ci_string make_ci_string(std::string_view string)
{
   return {string.data(), string.length()};
}

inline Ci_String_view view_as_ci_string(std::string_view string)
{
   return {string.data(), string.length()};
}

inline std::ostream& operator<<(std::ostream& stream, Ci_String_view string)
{
   return stream << std::string_view{string.data(), string.size()};
}

inline bool operator==(const Ci_String_view& l, const std::string_view& r)
{
   return l == view_as_ci_string(r);
}

inline bool operator==(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) == r;
}

inline bool operator!=(const Ci_String_view& l, const std::string_view& r)
{
   return l != view_as_ci_string(r);
}

inline bool operator!=(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) != r;
}

inline bool operator<(const Ci_String_view& l, const std::string_view& r)
{
   return l < view_as_ci_string(r);
}

inline bool operator<(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) < r;
}

inline bool operator<=(const Ci_String_view& l, const std::string_view& r)
{
   return l <= view_as_ci_string(r);
}

inline bool operator<=(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) <= r;
}

inline bool operator>(const Ci_String_view& l, const std::string_view& r)
{
   return l > view_as_ci_string(r);
}

inline bool operator>(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) > r;
}

inline bool operator>=(const Ci_String_view& l, const std::string_view& r)
{
   return l >= view_as_ci_string(r);
}

inline bool operator>=(const std::string_view& l, const Ci_String_view& r)
{
   return view_as_ci_string(l) >= r;
}
}

namespace std {
template<>
struct hash<sp::Ci_String_view> {
   using argument_type = sp::Ci_String_view;
   using result_type = std::uint64_t;

   result_type operator()(argument_type arg) const noexcept
   {
      constexpr std::uint64_t FNV_prime = 1099511628211;
      constexpr std::uint64_t offset_basis = 14695981039346656037;

      std::uint64_t hash = offset_basis;

      for (auto c : arg) {
         c = static_cast<char>(std::tolower(c));

         hash ^= c;
         hash *= FNV_prime;
      }

      return hash;
   }
};

template<>
struct hash<sp::Ci_string> {
   using argument_type = sp::Ci_string;
   using result_type = typename std::hash<sp::Ci_String_view>::result_type;

   result_type operator()(const argument_type& arg) const noexcept
   {
      return std::hash<sp::Ci_String_view>{}(arg);
   }
};
}
