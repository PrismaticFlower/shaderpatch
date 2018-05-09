#pragma once

#include "compose_exception.hpp"
#include "exceptions.hpp"
#include "file_helpers.hpp"
#include "string_utilities.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace sp {

inline auto parse_req_file(const std::filesystem::path& filepath,
                           std::string_view platform = "pc")
   -> std::vector<std::pair<std::string, std::vector<std::string>>>
{
   namespace fs = std::filesystem;
   using namespace std::literals;

   if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
      throw std::invalid_argument{"Attempt to open non-existent .req file "s};
   }

   auto buffer = load_string_file(filepath);

   // Filter out comments.
   buffer = std::regex_replace(buffer, std::regex{R"(//.+)"s}, ""s);

   std::istringstream stream{buffer};

   std::string header;

   stream >> header;

   if (header != "ucft"sv) {
      throw compose_exception<Parse_error>("Expected \"ucft\" but found "sv,
                                           std::quoted(header), '.');
   }

   std::string brace;

   stream >> brace;

   if (brace != "{"sv) {
      if (!stream) return {};

      throw compose_exception<Parse_error>("Expected opening '{' but found '"sv,
                                           brace, "'."sv);
   }

   std::vector<std::pair<std::string, std::vector<std::string>>> section_values;

   while (stream && !stream.eof()) {
      stream >> header;

      if (header == "}"sv) {
         break;
      }
      else if (!stream) {
         throw std::runtime_error{"Unexpected end of .req file."s};
      }
      else if (header != "REQN"sv) {
         throw compose_exception<Parse_error>("Unexpected token "sv,
                                              std::quoted(header),
                                              ", expected \"REQN\"");
      }

      stream >> brace;

      if (brace != "{"sv) {
         throw compose_exception<Parse_error>("Unexpected token '"sv, brace,
                                              "', expected '{'"sv);
      }

      if (!std::getline(stream >> std::ws, buffer, '}')) {
         throw Parse_error{"Unexpected end of .req file."s};
      }

      std::istringstream section{buffer};

      std::string section_type;

      section >> std::quoted(section_type);

      if (section_type.empty()) {
         throw Parse_error{"Unexpected empty REQN section."s};
      }

      std::vector<std::string> keys;
      keys.reserve(32);

      bool keep_keys = true;

      while (section && !section.eof()) {
         std::string key;

         section >> std::quoted(key) >> std::ws;

         if (key.empty()) continue;

         if (const auto split = split_string_on(key, "="sv); split[0] == "platform"sv) {
            if (split[1] != platform) keep_keys = false;
         }

         if (keep_keys) keys.emplace_back(std::move(key));
      }

      section_values.emplace_back(std::move(section_type), std::move(keys));
   }

   return section_values;
}

template<typename Container>
inline void parse_files_req_file(const std::filesystem::path& filepath,
                                 Container& container,
                                 typename Container::const_iterator insert_after)
{
   namespace fs = std::filesystem;
   using namespace std::literals;

   if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
      throw std::invalid_argument{"Attempt to open non-existent .req file "s};
   }

   auto buffer = load_string_file(filepath);

   // Filter out comments.
   buffer = std::regex_replace(buffer, std::regex{R"(//.+)"s}, ""s);

   std::istringstream stream{buffer};

   std::string header;

   stream >> header;

   if (header != "ucft"sv) {
      throw compose_exception<Parse_error>("Expected \"ucft\" but found "sv,
                                           std::quoted(header), '.');
   }

   std::string brace;

   stream >> brace;

   if (brace != "{"sv) {
      if (!stream) return;

      throw compose_exception<Parse_error>("Expected opening '{' but found '"sv,
                                           brace, "'."sv);
   }

   std::vector<std::pair<std::string, std::vector<std::string>>> section_values;

   while (stream && !stream.eof()) {
      stream >> header;

      if (header == "}"sv) {
         break;
      }
      else if (!stream) {
         throw Parse_error{"Unexpected end of .req file."s};
      }
      else if (header == "ANIM"sv) {
         // Really we should keep parsing but ANIM sections always seem to come
         // after a single FILE section, so to save some dev time we call it
         // quits here.

         return;
      }
      else if (header != "FILE"sv) {
         throw compose_exception<Parse_error>("Unexpected token "sv,
                                              std::quoted(header),
                                              ", expected \"FILE\"");
      }

      stream >> brace;

      if (brace != "{"sv) {
         throw compose_exception<Parse_error>("Unexpected token '"sv, brace,
                                              "', expected '{'"sv);
      }

      if (!std::getline(stream >> std::ws, buffer, '}')) {
         throw Parse_error{"Unexpected end of .req file."s};
      }

      std::istringstream section{buffer};

      while (section && !section.eof()) {
         std::string value;

         section >> std::quoted(value) >> std::ws;

         if (value.empty()) continue;

         container.insert(insert_after, value);
      }
   }
}

inline void emit_req_file(
   const std::filesystem::path& filepath,
   const std::vector<std::pair<std::string, std::vector<std::string>>>& key_sections)
{
   namespace fs = std::filesystem;
   using namespace std::literals;

   std::ofstream file{filepath};

   if (!file) {
      throw compose_exception<std::runtime_error>("Unable to open file "sv,
                                                  filepath, " for output."sv);
   }

   file << "ucft\n{\n"sv;

   for (const auto& key_section : key_sections) {
      file << "   "sv
           << "REQN\n   {\n"sv;

      file << "      "sv << std::quoted(key_section.first) << '\n';

      for (const auto& entry : key_section.second) {
         file << "      "sv << std::quoted(entry) << '\n';
      }

      file << "   }\n"sv;
   }

   file << "}\n"sv;
}
}
