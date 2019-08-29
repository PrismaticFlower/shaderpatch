#pragma once

#include <algorithm>
#include <filesystem>
#include <initializer_list>
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

inline void swap(Node& l, Node r) noexcept
{
   std::swap(l, r);
}

auto from_istream(std::istream& stream) -> Node;

auto to_ostream(std::ostream& stream, const Node& node) -> std::ostream&;

auto operator>>(std::istream& stream, Node& node) -> std::istream&;

auto operator<<(std::ostream& stream, const Node& node) -> std::ostream&;

auto load_file(const std::filesystem::path& path) -> cfg::Node;

void save_file(const cfg::Node& node, const std::filesystem::path& path);

template<typename Key_type>
inline auto find(const Node& node, const Key_type& key) -> Node::const_iterator
{
   return std::find_if(node.cbegin(), node.cend(), [&key](const auto& key_node) {
      return key_node.first == key;
   });
}

template<typename Key_type>
inline auto find(Node& node, const Key_type& key) -> Node::iterator
{
   return std::find_if(node.begin(), node.end(), [&key](const auto& key_node) {
      return key_node.first == key;
   });
}

}
