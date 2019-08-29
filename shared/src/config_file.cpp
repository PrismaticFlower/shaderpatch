
#include "config_file.hpp"
#include "compose_exception.hpp"
#include "string_utilities.hpp"

#include <fstream>
#include <iomanip>

namespace sp::cfg {

namespace {

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

std::ostream& operator<<(std::ostream& stream, const Indent& indent);

template<typename Values_range>
void print_values(std::ostream& stream, const Values_range& values);

template<typename Comments_range>
void print_comments(std::ostream& stream, int level, const Comments_range& comments);

std::ostream& to_ostream_impl(std::ostream& stream, const Node& node, int level);

}

auto from_istream(std::istream& stream) -> Node
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
            throw std::runtime_error{"Unexpected scope start!"s};
         }

         parse_scope(stream, root.back().second);
      }
      else {
         root.emplace_back(parse_key_entry(line));

         for (auto comment : comments) {
            root.back().second.emplace_comment(std::move(comment));
         }

         comments.clear();
      }
   }

   return root;
}

auto to_ostream(std::ostream& stream, const Node& node) -> std::ostream&
{
   return to_ostream_impl(stream, node, 0);
}

auto operator>>(std::istream& stream, Node& node) -> std::istream&
{
   node = from_istream(stream);

   return stream;
}

auto operator<<(std::ostream& stream, const Node& node) -> std::ostream&
{
   return to_ostream(stream, node);
}

auto load_file(const std::filesystem::path& path) -> cfg::Node
{
   std::ifstream file{path};

   try {
      return cfg::from_istream(file);
   }
   catch (std::exception& e) {
      throw compose_exception<std::runtime_error>(
         "Error occured while parsing ", path, ". Message: ", e.what());
   }
}

void save_file(const cfg::Node& node, const std::filesystem::path& path)
{
   std::ofstream file{path};

   file << node;
}

namespace {

auto parse_key_entry(std::string_view key_entry_comment) -> std::pair<std::string, Node>
{
   using namespace std::literals;

   Node node;

   auto [key_entry, comment] = split_string_on(key_entry_comment, "//"sv);

   node.trailing_comment(comment);

   auto [key, trail] = seperate_string_at(key_entry, "("sv);
   key = trim_whitespace(key);

   auto values_trail = sectioned_split_split(trail, "("sv, ")"sv);

   if (!values_trail) {
      throw compose_exception<std::runtime_error>("Error parsing key "sv,
                                                  std::quoted(key), '.');
   }

   auto values = tokenize_string_on(values_trail.value().at(0), ","sv);

   for (auto value : values) {
      try {
         parse_value(value, node);
      }
      catch (std::exception& e) {
         throw compose_exception<std::runtime_error>("Error parsing key "sv,
                                                     std::quoted(key), ' ', e.what());
      }
   }

   return {std::string{key}, node};
}

void parse_value(std::string_view value, Node& node)
{
   using namespace std::literals;

   value = trim_whitespace(value);

   if (begins_with(value, "\""sv)) {
      if (!ends_with(value, "\""sv)) {
         throw compose_exception<std::runtime_error>("String missing closing quote."sv);
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

void parse_scope(std::istream& stream, Node& parent)
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
            throw std::runtime_error{"Unexpected scope start!"s};
         }

         parse_scope(stream, parent.back().second);
      }
      else if (begins_with(line, "}"sv)) {
         return;
      }
      else {
         parent.emplace_back(parse_key_entry(line));

         for (auto comment : comments) {
            parent.back().second.emplace_comment(std::move(comment));
         }

         comments.clear();
      }
   }

   throw std::runtime_error{
      "Reached end of stream inside scope without finding closing bracket!"s};
}

std::ostream& operator<<(std::ostream& stream, const Indent& indent)
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

std::ostream& to_ostream_impl(std::ostream& stream, const Node& node, int level = 0)
{
   using namespace std::literals;

   for (auto& child : node) {
      print_comments(stream, level, child.second.comments());

      stream << Indent{level} << child.first << '(';

      print_values(stream, child.second.values());

      stream << ')';

      if (!child.second.trailing_comment().empty()) {
         stream << " // "sv << child.second.trailing_comment();
      }

      stream << '\n';

      if (child.second.size() == 0) continue;

      stream << Indent{level} << "{\n"sv;

      to_ostream_impl(stream, child.second, level + 1);

      stream << Indent{level} << "}\n\n"sv;
   }

   return stream;
}

}

}
