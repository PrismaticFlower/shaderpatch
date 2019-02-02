#include "req_file_helpers.hpp"
#include "compose_exception.hpp"
#include "file_helpers.hpp"
#include "string_utilities.hpp"

#include <fstream>
#include <iomanip>
#include <regex>
#include <stdexcept>

namespace sp {

auto parse_req_file(const std::filesystem::path& filepath, std::string_view platform)
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
      throw compose_exception<std::runtime_error>("Expected \"ucft\" but found "sv,
                                                  std::quoted(header), '.');
   }

   std::string brace;

   stream >> brace;

   if (brace != "{"sv) {
      if (!stream) return {};

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

      if (brace != "{"sv) {
         throw compose_exception<std::runtime_error>("Unexpected token '"sv,
                                                     brace, "', expected '{'"sv);
      }

      if (!std::getline(stream >> std::ws, buffer, '}')) {
         throw std::runtime_error{"Unexpected end of .req file."s};
      }

      std::istringstream section{buffer};

      std::string section_type;

      section >> section_type >> std::ws;

      if (section_type.empty()) continue;

      if (auto unquoted = sectioned_split_split(section_type, "\""sv, "\""sv); unquoted) {
         section_type = (*unquoted)[0];
      }
      else {
         throw compose_exception<std::runtime_error>("Unexpected token '"sv, section_type,
                                                     "', expected section type"sv);
      }

      std::vector<std::string> keys;
      keys.reserve(32);

      bool keep_keys = true;

      while (section && !section.eof()) {
         std::string key;

         std::getline(section, key);
         section >> std::ws;

         if (auto unquoted = sectioned_split_split(key, "\""sv, "\""sv); unquoted) {
            key = (*unquoted)[0];
         }
         else {
            throw compose_exception<std::runtime_error>("Unexpected token '"sv, key,
                                                        "', expected section entry."sv);
         }

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

void emit_req_file(const std::filesystem::path& filepath,
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

void parse_files_req_file(const std::filesystem::path& filepath,
                          Small_function<void(std::string entry) noexcept> callback)
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
      throw compose_exception<std::runtime_error>("Expected \"ucft\" but found "sv,
                                                  std::quoted(header), '.');
   }

   std::string brace;

   stream >> brace;

   if (brace != "{"sv) {
      if (!stream) return;

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
      else if (header == "ANIM"sv) {
         // Really we should keep parsing but ANIM sections always seem to come
         // after a single FILE section, so to save some dev time we call it
         // quits here.

         return;
      }
      else if (header != "FILE"sv) {
         throw compose_exception<std::runtime_error>("Unexpected token "sv,
                                                     std::quoted(header),
                                                     ", expected \"FILE\"");
      }

      stream >> brace;

      if (brace != "{"sv) {
         throw compose_exception<std::runtime_error>("Unexpected token '"sv,
                                                     brace, "', expected '{'"sv);
      }

      if (!std::getline(stream >> std::ws, buffer, '}')) {
         throw std::runtime_error{"Unexpected end of .req file."s};
      }

      std::istringstream section{buffer};

      while (section && !section.eof()) {
         std::string value;

         std::getline(section, value);
         section >> std::ws;

         if (auto unquoted = sectioned_split_split(value, "\""sv, "\""sv); unquoted) {
            value = (*unquoted)[0];
         }
         else {
            throw compose_exception<std::runtime_error>("Unexpected token '"sv, value,
                                                        "', expected section entry."sv);
         }

         if (value.empty()) continue;

         callback(std::move(value));
      }
   }
}

}
