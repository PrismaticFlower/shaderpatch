#pragma once

#include "compose_exception.hpp"

#include <fstream>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <boost/filesystem.hpp>

namespace sp {

inline auto parse_req_file(boost::filesystem::path filepath)
   -> std::vector<std::pair<std::string, std::vector<std::string>>>
{
   namespace fs = boost::filesystem;
   using namespace std::literals;

   if (!fs::exists(filepath) || !fs::is_regular_file(filepath)) {
      throw std::invalid_argument{"Attempt to open non-existent .req file "s};
   }

   std::string buffer;

   fs::load_string_file(filepath, buffer);

   // Filter out comments.
   buffer = std::regex_replace(buffer, std::regex{R"(//.+)"s}, ""s);

   std::istringstream stream{buffer};

   std::string header;

   stream >> header;

   if (header != "ucft"sv) {
      throw compose_exception<std::runtime_error>("Expected \"ucft\" but found "sv,
                                                  std::quoted(header), '.');
   }

   char brace;

   stream >> brace;

   if (brace != '{') {
      throw compose_exception<std::runtime_error>("Expected opening '{' but found '"sv,
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
         throw compose_exception<std::runtime_error>("Unexpected token "sv,
                                                     std::quoted(header),
                                                     ", expected \"REQN\"");
      }

      stream >> brace;

      if (brace != '{') {
         throw compose_exception<std::runtime_error>("Unexpected token '"sv,
                                                     brace, "', expected '{'"sv);
      }

      if (!std::getline(stream >> std::ws, buffer, '}')) {
         throw std::runtime_error{"Unexpected end of .req file."s};
      }

      std::istringstream section{buffer};

      std::string section_type;

      section >> std::quoted(section_type);

      if (section_type.empty()) {
         throw std::runtime_error{"Unexpected empty REQN section."s};
      }

      std::vector<std::string> keys;
      keys.reserve(32);

      while (section && !section.eof()) {
         keys.emplace_back();

         section >> std::quoted(keys.back()) >> std::ws;
      }

      section_values.emplace_back(std::move(section_type), std::move(keys));
   }

   return section_values;
}

inline void emit_req_file(
   boost::filesystem::path filepath,
   const std::vector<std::pair<std::string, std::vector<std::string>>>& key_sections)
{
   namespace fs = boost::filesystem;
   using namespace std::literals;

   std::ofstream file{filepath.string()};

   if (!file) {
      throw compose_exception<std::runtime_error>("Unable to open file "sv,
                                                  filepath, " for output."sv);
   }

   file << "uctf\n{\n"sv;

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
