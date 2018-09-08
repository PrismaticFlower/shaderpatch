#pragma once

#include "compose_exception.hpp"
#include "exceptions.hpp"
#include "string_utilities.hpp"

#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace sp::cfg {

class Node {
public:
   using value_type = std::pair<std::string, Node>;
   using reference = value_type&;
   using const_reference = const value_type&;
   using iterator = std::vector<std::pair<std::string, Node>>::iterator;
   using const_iterator = std::vector<std::pair<std::string, Node>>::const_iterator;
   using difference_type = std::vector<std::pair<std::string, Node>>::difference_type;
   using size_type = std::vector<std::pair<std::string, Node>>::size_type;

   using Node_value = std::variant<std::string, long long, double>;

   Node() = default;

   Node(size_type count, const_reference value)
   {
      _children.assign(count, value);
   }

   Node(const_iterator first, const_iterator last)
   {
      _children.assign(first, last);
   }

   Node(std::initializer_list<value_type> nodes)
   {
      _children.assign(std::cbegin(nodes), std::cend(nodes));
   }

   auto begin() noexcept -> iterator
   {
      return _children.begin();
   }

   auto begin() const noexcept -> const_iterator
   {
      return _children.begin();
   }

   auto end() noexcept -> iterator
   {
      return _children.end();
   }

   auto end() const noexcept -> const_iterator
   {
      return _children.end();
   }

   auto cbegin() const noexcept -> const_iterator
   {
      return _children.cbegin();
   }

   auto cend() const noexcept -> const_iterator
   {
      return _children.cend();
   }

   auto swap(Node& with) noexcept
   {
      std::swap(*this, with);
   }

   auto size() const noexcept -> size_type
   {
      return _children.size();
   }

   auto max_size() const noexcept -> size_type
   {
      return _children.max_size();
   }

   bool empty() const noexcept
   {
      return _children.empty();
   }

   template<typename... Args>
   auto emplace(const_iterator where, Args... args) -> iterator
   {
      return _children.emplace(where, std::forward<Args>(args)...);
   }

   auto insert(const_iterator where, const_reference value) -> iterator
   {
      return _children.insert(where, value);
   }

   auto insert(const_iterator where, value_type&& value) -> iterator
   {
      return _children.insert(where, std::move(value));
   }

   auto insert(const_iterator where, size_type count, const_reference value) -> iterator
   {
      return _children.insert(where, count, value);
   }

   auto insert(const_iterator where, const_iterator first, const_iterator last) -> iterator
   {
      return _children.insert(where, first, last);
   }

   auto insert(const_iterator where, std::initializer_list<value_type> nodes) -> iterator
   {
      return _children.insert(where, std::cbegin(nodes), std::cend(nodes));
   }

   auto erase(const_iterator where) -> iterator
   {
      return _children.erase(where);
   }

   auto erase(const_iterator first, const_iterator last) -> iterator
   {
      return _children.erase(first, last);
   }

   void clear()
   {
      _children.clear();
   }

   void assign(const_iterator first, const_iterator last)
   {
      return _children.assign(first, last);
   }

   void assign(std::initializer_list<value_type> nodes)
   {
      return _children.assign(std::cbegin(nodes), std::cend(nodes));
   }

   void assign(size_type count, const_reference value)
   {
      return _children.assign(count, value);
   }

   auto front() noexcept -> reference
   {
      return _children.front();
   }

   auto front() const noexcept -> const_reference
   {
      return _children.front();
   }

   auto back() noexcept -> reference
   {
      return _children.back();
   }

   auto back() const noexcept -> const_reference
   {
      return _children.back();
   }

   template<typename... Args>
   auto emplace_back(Args... args) -> reference
   {
      return _children.emplace_back(std::forward<Args>(args)...);
   }

   void push_back(const_reference value)
   {
      _children.push_back(value);
   }

   void push_back(value_type&& value)
   {
      _children.push_back(std::move(value));
   }

   void pop_back()
   {
      _children.pop_back();
   }

   template<typename First, typename... Args>
   void emplace_value(First&& first, Args&&... args)
   {
      if constexpr (sizeof...(args) == 0) {
         if constexpr (std::is_floating_point_v<First>) {
            _values.emplace_back(std::in_place_type<double>,
                                 std::forward<First>(first));
         }
         else if constexpr (std::is_integral_v<First>) {
            _values.emplace_back(std::in_place_type<long long>,
                                 std::forward<First>(first));
         }
         else if constexpr (std::is_same_v<std::decay_t<First>, std::string_view>) {
            _values.emplace_back(std::in_place_type<std::string>,
                                 std::forward<First>(first));
         }
         else if constexpr (std::is_convertible_v<std::decay_t<First>, std::string>) {
            _values.emplace_back(std::in_place_type<std::string>,
                                 std::forward<First>(first));
         }
         else {
            static_assert(false,
                          "Unable to construct config value from argument.");
         }
      }
      else {
         _values.emplace_back(std::forward<First>(first), std::forward<Args>(args)...);
      }
   }

   auto values() const noexcept -> const std::vector<Node_value>&
   {
      return _values;
   }

   auto values_count() const noexcept -> size_type
   {
      return _values.size();
   }

   template<typename Value_type>
   auto get_value(int index = 0) const noexcept -> Value_type
   {
      auto val = _values.at(index);

      if constexpr (std::is_arithmetic_v<Value_type>) {
         if (std::holds_alternative<long long>(val)) {
            return static_cast<Value_type>(std::get<long long>(val));
         }
         else {
            return static_cast<Value_type>(std::get<double>(val));
         }
      }
      else if constexpr (std::is_same_v<Value_type, std::string>) {
         if (std::holds_alternative<long long>(val)) {
            return std::to_string(std::get<long long>(val));
         }
         else if (std::holds_alternative<double>(val)) {
            return std::to_string(std::get<double>(val));
         }
         else {
            return static_cast<Value_type>(std::get<std::string>(val));
         }
      }
      else {
         static_assert(false, "Unknown Value_type.");
      }
   }

   template<typename Value_type>
   void set_value(const Value_type& value, int index = 0) noexcept
   {
      if constexpr (std::is_floating_point_v<Value_type>) {
         _values.at(index) = static_cast<double>(value);
      }
      else if constexpr (std::is_integral_v<Value_type>) {
         _values.at(index) = static_cast<long long>(value);
      }
      else if constexpr (std::is_same_v<Value_type, std::string> ||
                         std::is_same_v<Value_type, std::string_view>) {
         _values.at(index) = static_cast<std::string>(value);
      }
      else {
         static_assert(false, "Unknown Value_type.");
      }
   }

   template<typename... Args>
   void emplace_comment(Args... args) noexcept
   {
      _comments.emplace_back(std::forward<Args>(args)...);
   }

   auto comments() const noexcept -> const std::vector<std::string>&
   {
      return _comments;
   }

   void trailing_comment(std::string_view comment)
   {
      _trailing_comment = std::string{comment};
   }

   auto trailing_comment() const noexcept -> std::string_view
   {
      return _trailing_comment;
   }

private:
   std::vector<Node_value> _values;
   std::vector<std::pair<std::string, Node>> _children;

   std::vector<std::string> _comments;
   std::string _trailing_comment;
};

void swap(Node& l, Node r) noexcept
{
   std::swap(l, r);
}

namespace detail {

auto parse_key_entry(std::string_view key_entry_comment)
   -> std::pair<std::string, Node>;

void parse_value(std::string_view value, Node& node);

void parse_scope(std::istream& stream, Node& parent);

struct Indent {
   Indent(int level) : count{level * width} {}

   constexpr static char ws = ' ';
   constexpr static int width = 3;
   int count = 0;
};

inline std::ostream& operator<<(std::ostream& stream, const Indent& indent);

template<typename Values_range>
void print_values(std::ostream& stream, const Values_range& values);

template<typename Comments_range>
void print_comments(std::ostream& stream, int level, const Comments_range& comments);

}

inline Node from_istream(std::istream& stream)
{
   using namespace std::literals;

   Node root;

   std::vector<std::string> comments;

   stream >> std::ws;

   for (std::string line; stream.good(); stream >> std::ws) {
      std::getline(stream, line);

      if (line.empty()) continue;

      if (begins_with(line, "//"sv)) {
         comments.emplace_back(std::cbegin(line) + 2, std::cend(line));
      }
      else if (begins_with(line, "{"sv)) {
         if (root.empty()) {
            throw Parse_error{"Unexpected scope start!"s};
         }

         detail::parse_scope(stream, root.back().second);
      }
      else {
         root.emplace_back(detail::parse_key_entry(line));

         for (auto comment : comments) {
            root.back().second.emplace_comment(std::move(comment));
         }

         comments.clear();
      }
   }

   return root;
}

inline std::ostream& to_ostream(std::ostream& stream, const Node& node, int level = 0)
{
   using detail::Indent;
   using namespace std::literals;

   for (auto& child : node) {
      detail::print_comments(stream, level, child.second.comments());

      stream << Indent{level} << child.first << '(';

      detail::print_values(stream, child.second.values());

      stream << ')';

      if (!child.second.trailing_comment().empty()) {
         stream << " // "sv << child.second.trailing_comment();
      }

      stream << '\n';

      if (child.second.size() == 0) continue;

      stream << Indent{level} << "{\n"sv;

      to_ostream(stream, child.second, level + 1);

      stream << Indent{level} << "}\n\n"sv;
   }

   return stream;
}

inline std::istream& operator>>(std::istream& stream, Node& node)
{
   node = from_istream(stream);
}

inline std::ostream& operator<<(std::ostream& stream, const Node& node)
{
   return to_ostream(stream, node, 0);
}

namespace detail {

inline auto parse_key_entry(std::string_view key_entry_comment)
   -> std::pair<std::string, Node>
{
   using namespace std::literals;

   Node node;

   auto [key_entry, comment] = split_string_on(key_entry_comment, "//"sv);

   node.trailing_comment(comment);

   auto [key, trail] = seperate_string_at(key_entry, "("sv);
   key = trim_whitespace(key);

   auto values_trail = sectioned_split_split(trail, "("sv, ")"sv);

   if (!values_trail) {
      throw compose_exception<Parse_error>("Error parsing key "sv,
                                           std::quoted(key), '.');
   }

   auto values = tokenize_string_on(values_trail.value().at(0), ","sv);

   for (auto value : values) {
      try {
         parse_value(value, node);
      }
      catch (std::exception& e) {
         throw compose_exception<Parse_error>("Error parsing key "sv,
                                              std::quoted(key), ' ', e.what());
      }
   }

   return {std::string{key}, node};
}

inline void parse_value(std::string_view value, Node& node)
{
   using namespace std::literals;

   value = trim_whitespace(value);

   if (begins_with(value, "\""sv)) {
      if (!ends_with(value, "\""sv)) {
         throw compose_exception<Parse_error>("String missing closing quote."sv);
      }

      value.remove_prefix(1);
      value.remove_suffix(1);

      node.emplace_value(value);
   }
   else if (contains(value, "."sv)) {
      node.emplace_value(std::stod(std::string{value}));
   }
   else {
      node.emplace_value(std::stoll(std::string{value}));
   }
}

inline void parse_scope(std::istream& stream, Node& parent)
{
   using namespace std::literals;

   std::vector<std::string> comments;

   stream >> std::ws;

   for (std::string line; stream.good(); stream >> std::ws) {
      std::getline(stream, line);

      if (line.empty()) continue;

      if (begins_with(line, "//"sv)) {
         comments.emplace_back(std::cbegin(line) + 2, std::cend(line));
      }
      else if (begins_with(line, "{"sv)) {
         if (parent.empty()) {
            throw Parse_error{"Unexpected scope start!"s};
         }

         parse_scope(stream, parent.back().second);
      }
      else if (begins_with(line, "}"sv)) {
         return;
      }
      else {
         parent.emplace_back(detail::parse_key_entry(line));

         for (auto comment : comments) {
            parent.back().second.emplace_comment(std::move(comment));
         }

         comments.clear();
      }
   }

   throw Parse_error{"Reached end of stream inside scope without finding closing bracket!"s};
}

inline std::ostream& operator<<(std::ostream& stream, const Indent& indent)
{
   for (int i = 0; i < indent.count; ++i) {
      stream.put(indent.ws);
   }

   return stream;
}

template<typename Values_range>
void print_values(std::ostream& stream, const Values_range& values)
{
   using namespace std::literals;

   bool first = true;

   for (const auto& v : values) {
      if (!std::exchange(first, false)) {
         stream << ", "sv;
      }

      std::visit(
         [&](auto val) {
            using Type = std::decay_t<decltype(val)>;

            if constexpr (std::is_same_v<Type, long long>) {
               stream << val;
            }
            else if constexpr (std::is_same_v<Type, double>) {
               stream << std::setprecision(5) << val;
            }
            else if constexpr (std::is_same_v<Type, std::string>) {
               stream << std::quoted(val);
            }
            else

            {
               static_assert(false, "Unexpected Node_value type!");
            }
         },
         v);
   }
}

template<typename Comments_range>
void print_comments(std::ostream& stream, int level, const Comments_range& comments)
{
   using namespace std::literals;

   for (const auto& comment : comments) {
      stream << Indent{level} << "// "sv << comment << '\n';
   }
}
}

}
