#pragma once

#include <array>
#include <cctype>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gsl/gsl>

namespace sp {

//! \brief Splits a string along a delimiter, exclusively.
//!
//! \param string The string view to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return Two string views of either side of the delimiter. If the delimiter
//! was not found, the second view is empty.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string_on(
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

//! \brief Splits a string along a delimiter, exclusively.
//!
//! \param string The string to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return Two string views of either side of the delimiter. If the delimiter
//! was not found, the second view is empty.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string_on(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return split_string_on(std::basic_string_view<Char_t, Char_triats>{string}, delimiter);
}

//! \brief Splits a string along a delimiter, inclusively. The delimiter
//! will be included in the second returned string.
//!
//! \param string The string view to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return Two string views of either side of the delimiter. If the delimiter
//! was not found, the second view is empty.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto seperate_string_at(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   const auto offset = string.find_first_of(delimiter);

   if (offset == string.npos) return {string, std::string_view{""}};

   std::array<std::basic_string_view<Char_t, Char_triats>, 2> split_strings;

   split_strings[0] = string.substr(0, offset);
   split_strings[1] = string.substr(offset, string.length() - offset);

   return split_strings;
}

//! \brief Splits a string along a delimiter, inclusively. The delimiter
//! will be included in the second returned string.
//!
//! \param string The string to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return Two string views of either side of the delimiter. If the delimiter
//! was not found, the second view is empty.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto seperate_string_at(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return seperate_string_at(std::basic_string_view<Char_t, Char_triats>{string},
                             delimiter);
}

//! \brief Splits a string along a delimiter, repeatadly and exclusively.
//!
//! \param string The string view to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return A vector string views split along the delimiter.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
inline auto tokenize_string_on(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter)
   -> std::vector<std::string_view>
{
   std::vector<std::string_view> parts;

   for (auto offset = string.find_first_of(delimiter); offset != string.npos;
        offset = string.find_first_of(delimiter)) {

      parts.emplace_back(string.substr(0, offset));
      string = string.substr(offset + delimiter.length(),
                             string.length() - offset - delimiter.length());
   }

   if (!string.empty()) parts.emplace_back(string);

   return parts;
}

//! \brief Splits a string along a delimiter, repeatadly and exclusively.
//!
//! \param string The string view to split.
//! \param delimiter The delimiter to split the string on.
//!
//! \return A vector string views split along the delimiter.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
inline auto tokenize_string_on(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::vector<std::string_view>
{
   return tokenize_string_on(std::basic_string_view<Char_t, Char_triats>{string},
                             delimiter);
}

//! \brief Checks if a string begins with another string.
//!
//! \param string The string view to split.
//! \param what The string to search check for.
//!
//! \return True if the string begeins with what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool begins_with(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   if (what.size() > string.size()) return false;

   return (string.substr(0, what.size()) == what);
}

//! \brief Checks if a string begins with another string.
//!
//! \param string The string to split.
//! \param what The string to search check for.
//!
//! \return True if the string ends with what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool begins_with(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   return begins_with(std::basic_string_view<Char_t, Char_triats>{string}, what);
}

//! \brief Checks if a string ends with another string.
//!
//! \param string The string view to split.
//! \param what The string to search check for.
//!
//! \return True if the string begeins with what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool ends_with(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   if (what.size() > string.size()) return false;

   return (string.substr(string.size() - what.size(), what.size()) == what);
}

//! \brief Checks if a string ends with another string.
//!
//! \param string The string to split.
//! \param what The string to search check for.
//!
//! \return True if the string ends with what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool ends_with(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   return begins_with(std::basic_string_view<Char_t, Char_triats>{string}, what);
}

//! \brief Checks if a string contains another string.
//!
//! \param string The string view to split.
//! \param what The string to search check for.
//!
//! \return True if the string contains what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool contains(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   return (string.find(what) != string.npos);
}

//! \brief Checks if a string contains another string.
//!
//! \param string The string to split.
//! \param what The string to search check for.
//!
//! \return True if the string contains what, false if it doesn't.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr bool contains(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type what) noexcept
{
   return contains(std::basic_string_view<Char_t, Char_triats>{string}, what);
}

//! \brief Splits a string into two parts using section delimiters.
//!
//! \param string The string view to split.
//! \param open The opening delimiter for the section. The string must start
//! with this.
//! \param close The closing delimiter for the section.
//!
//! \return An optional value containing the resulting strings if the split
//! was successful.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto sectioned_split_split(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type open,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type close = open) noexcept
   -> std::optional<std::array<std::basic_string_view<Char_t, Char_triats>, 2>>
{
   if (!begins_with(string, open)) return std::nullopt;

   string = split_string_on(string, open)[1];

   auto [section, remainder] = seperate_string_at(string, close);

   if (!begins_with(remainder, close)) return std::nullopt;

   remainder.remove_prefix(close.size());

   return std::array<std::basic_string_view<Char_t, Char_triats>, 2>{section, remainder};
}

//! \brief Splits a string into two parts using section delimiters.
//!
//! \param string The string view to split.
//! \param open The opening delimiter for the section. The string must start
//! with this.
//! \param close The closing delimiter for the section.
//!
//! \return An optional value containing the resulting strings if the split
//! was successful.
template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto sectioned_split_split(
   std::basic_string<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type open,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type close) noexcept
   -> std::optional<std::array<std::basic_string_view<Char_t, Char_triats>, 2>>
{
   return sectioned_split_split(std::basic_string_view<Char_t, Char_triats>{string},
                                open, close);
}

//! \brief Trim leading and trailing whitespace from a string view.
//!
//! \param string The string view to trim whitespace from.
//!
//! \return The resulting string view.
template<typename Char_triats = std::char_traits<char>>
constexpr auto trim_whitespace(std::basic_string_view<char, Char_triats> string) noexcept
   -> std::basic_string_view<char, Char_triats>
{
   const auto not_ws = [](auto c) { return std::isspace(c) == 0; };

   {
      auto result = std::find_if(std::begin(string), std::end(string), not_ws);

      string.remove_prefix(
         gsl::narrow_cast<std::size_t>(std::distance(std::begin(string), result)));
   }
   {
      auto result = std::find_if(std::rbegin(string), std::rend(string), not_ws);

      string.remove_suffix(
         gsl::narrow_cast<std::size_t>(std::distance(std::rbegin(string), result)));
   }

   return string;
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

constexpr Ci_String_view operator""_svci(const char* chars, std::size_t size) noexcept
{
   return Ci_String_view{chars, size};
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
