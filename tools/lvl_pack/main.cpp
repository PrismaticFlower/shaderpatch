
#include "req_file_helpers.hpp"
#include "synced_io.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <gsl/gsl>

#include <clara.hpp>

namespace fs = boost::filesystem;

using namespace sp;
using namespace std::literals;

namespace {

constexpr std::uint32_t fnv_1a_hash(std::string_view str)
{
   constexpr std::uint32_t FNV_prime = 16777619;
   constexpr std::uint32_t offset_basis = 2166136261;

   std::uint32_t hash = offset_basis;

   for (auto c : str) {
      c |= 0x20;

      hash ^= c;
      hash *= FNV_prime;
   }

   return hash;
}

constexpr Magic_number lvl_name_hash(std::string_view str)
{
   return static_cast<Magic_number>(fnv_1a_hash(str));
}

void write_file_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                       std::unordered_set<fs::path::string_type>& added_files,
                       const std::unordered_map<std::string, fs::path>& file_map_list,
                       const fs::path& filepath);

void write_req_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                      std::unordered_set<fs::path::string_type>& added_files,
                      const std::unordered_map<std::string, fs::path>& file_map_list,
                      std::vector<std::pair<std::string, std::vector<std::string>>> req_file_contents);

auto read_binary_in(const fs::path& path) -> std::vector<std::byte>
{
   Expects(fs::exists(path) && fs::is_regular_file(path));

   std::ifstream file{path.string(), std::ios::binary | std::ios::ate};

   const auto file_size = file.tellg();

   std::vector<std::byte> vector;
   vector.resize(static_cast<std::size_t>(file_size));

   file.seekg(0);
   file.read(reinterpret_cast<char*>(vector.data()), vector.size());

   return vector;
}

auto build_file_map_list(const std::vector<std::string>& source_directories) noexcept
   -> std::unordered_map<std::string, fs::path>
{
   std::unordered_map<std::string, fs::path> files;

   for (auto& dir : source_directories) {
      fs::path source_path{dir};

      if (!fs::exists(source_path) || !fs::is_directory(source_path)) {
         synced_error_print("Warning source directory "sv, source_path,
                            " does not exist!"sv);

         continue;
      }

      for (auto& entry : fs::recursive_directory_iterator{source_path}) {
         const auto filename = entry.path().filename().string();

         if (!fs::is_regular_file(entry.path()) ||
             fs::extension(entry.path()) == ".req"sv || files.count(filename)) {
            continue;
         }

         files[filename] = entry.path();
      }
   }

   return files;
}

void write_file_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                       std::unordered_set<fs::path::string_type>& added_files,
                       const std::unordered_map<std::string, fs::path>& file_map_list,
                       const fs::path& filepath)
{
   const auto filename = filepath.filename().native();

   if (added_files.count(filename)) return;

   const auto req_path =
      fs::change_extension(filepath, fs::extension(filepath) += ".req"sv);

   if (fs::exists(req_path) && fs::is_regular_file(req_path)) {
      write_req_to_lvl(req_file_path, writer, added_files, file_map_list,
                       parse_req_file(req_path));
   }

   const auto file_data = read_binary_in(filepath);

   ucfb::Reader reader{gsl::make_span(file_data)};

   if (filepath.extension() == ".lvl"s) {
      auto lvl_writer = writer.emplace_child("lvl_"_mn);

      lvl_writer.emplace_child(lvl_name_hash(filepath.filename().string()))
         .write(reader.read_array<std::byte>(reader.size()));
   }
   else {
      writer.write(reader.read_array<std::byte>(reader.size()));
   }

   added_files.emplace(filename);
}

void write_req_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                      std::unordered_set<fs::path::string_type>& added_files,
                      const std::unordered_map<std::string, fs::path>& file_map_list,
                      std::vector<std::pair<std::string, std::vector<std::string>>> req_file_contents)
{
   for (auto& section : req_file_contents) {
      for (auto& value : section.second) {
         const auto filename = value + "."s + section.first;
         if (!file_map_list.count(filename)) {
            synced_error_print("Warning nonexistent file "sv, std::quoted(filename),
                               " referenced in "sv, req_file_path, '.');

            continue;
         }

         write_file_to_lvl(req_file_path, writer, added_files, file_map_list,
                           file_map_list.at(filename));
      }
   }
}

void build_lvl_file(const fs::path& req_file_path, const fs::path& output_directory,
                    const std::unordered_map<std::string, fs::path>& file_map_list) noexcept
{
   Expects(fs::exists(req_file_path) && fs::exists(output_directory));

   const auto output_path =
      output_directory / fs::change_extension(req_file_path.filename(), ".lvl"s);

   try {
      bool success = false;
      auto file_cleaner = gsl::finally([&success, &output_path] {
         boost::system::error_code err;

         if (!success) fs::remove(output_path, err);
      });

      auto file = ucfb::open_file_for_output(output_path.string());
      ucfb::Writer writer{file};

      std::unordered_set<fs::path::string_type> added_files;

      write_req_to_lvl(req_file_path, writer, added_files, file_map_list,
                       parse_req_file(req_file_path));

      success = true;
   }
   catch (std::exception& e) {
      synced_error_print("Error occured while packing "sv,
                         req_file_path.filename(), "\n   ", e.what());
   }
}
}

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   auto input_dir = "./"s;
   std::vector<std::string> source_directories;
   bool recursive = false;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place resulting .lvl files in."s)
      | Opt{input_dir, "input directory"s}
      ["--inputdir"s]
      ("Input directory for .req files"s)
      | Opt{source_directories, "source directory"s}
      ["--sourcedir"s]["-s"s]
      ("Specify a source directory for munged files. Multiple source" 
       " directories can be used and should be listed in order of precedence."s)
      | Opt{recursive, "recursive"s}
      ["-r"s]["--recursive"s]
      ("Search input directory recursively for .req files."s);

   // clang-format on

   const auto result = cli.parse(Args(arg_count, args));

   if (!result) {
      synced_error_print("Commandline Error: "sv, result.errorMessage());

      return 1;
   }
   else if (help) {
      synced_print(cli);

      return 0;
   }

   if (!fs::exists(output_dir) && !fs::create_directory(output_dir)) {
      synced_error_print("Unable to create output directory "sv,
                         std::quoted(output_dir), '.');

      return 1;
   }

   if (!fs::exists(input_dir)) {
      synced_error_print("Input Directory "sv, std::quoted(input_dir),
                         " does not exist!"sv);

      return 1;
   }

   if (source_directories.empty()) {
      synced_error_print("No source directories specified!"sv);

      return 1;
   }

   const auto file_map_list = build_file_map_list(source_directories);

   const auto process_directory = [&](auto&& iterator) {
      for (auto& entry : std::forward<decltype(iterator)>(iterator)) {
         if (!fs::is_regular_file(entry.path())) continue;

         if (const auto ext = fs::extension(entry); ext != ".req"sv && ext != ".mrq"sv) {
            continue;
         }

         synced_print("Munging lvl "sv, entry.path().filename().string(), "..."sv);

         build_lvl_file(entry.path(), output_dir, file_map_list);
      }
   };

   if (recursive) {
      process_directory(fs::recursive_directory_iterator{input_dir});
   }
   else {
      process_directory(fs::directory_iterator{input_dir});
   }
}
