
#include "req_file_helpers.hpp"
#include "synced_io.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
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

auto normalize_path(fs::path path) -> fs::path::string_type
{
   path = path.lexically_normal().make_preferred();

   fs::path::string_type string;
   string.reserve(path.native().size());

   std::transform(std::cbegin(path.native()), std::cend(path.native()),
                  std::back_inserter(string), std::towlower);

   return string;
}

auto make_extern_files_set(const std::vector<std::string>& extern_files_list_paths)
   -> std::unordered_set<std::string>
{
   std::unordered_set<std::string> extern_files;

   for (fs::path path : extern_files_list_paths) {
      if (!fs::exists(path) || !fs::is_regular_file(path)) {
         synced_error_print("Warning extern files list "sv, path, " does not exist!"sv);

         continue;
      }

      parse_files_req_file(path, extern_files, std::cbegin(extern_files));
   }

   return extern_files;
}

void write_file_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                       std::unordered_set<fs::path::string_type>& added_files,
                       const std::vector<fs::path>& source_dirs,
                       const std::unordered_set<std::string>& extern_files,
                       const fs::path& filepath);

void write_req_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                      std::unordered_set<fs::path::string_type>& added_files,
                      const std::vector<fs::path>& source_dirs,
                      const std::unordered_set<std::string>& extern_files,
                      std::vector<std::pair<std::string, std::vector<std::string>>> req_file_contents);

auto find_file(const std::string& filename, const std::vector<fs::path>& source_dirs)
   -> std::optional<fs::path>
{
   for (const auto& dir : source_dirs) {
      const auto path = dir / filename;

      if (!fs::exists(path) || !fs::is_regular_file(path)) continue;

      return path;
   }

   return std::nullopt;
}

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

void write_file_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                       std::unordered_set<fs::path::string_type>& added_files,
                       const std::vector<fs::path>& source_dirs,
                       const std::unordered_set<std::string>& extern_files,
                       const fs::path& filepath)
{
   const auto normalized_path = normalize_path(filepath);

   if (added_files.count(normalized_path)) return;

   const auto req_path =
      fs::change_extension(filepath, fs::extension(filepath) += ".req"sv);

   if (fs::exists(req_path) && fs::is_regular_file(req_path)) {
      write_req_to_lvl(req_file_path, writer, added_files, source_dirs,
                       extern_files, parse_req_file(req_path));
   }

   const auto file_data = read_binary_in(filepath);

   ucfb::Reader reader{gsl::make_span(file_data)};

   if (filepath.extension() == ".lvl"s) {
      auto lvl_writer = writer.emplace_child("lvl_"_mn);

      lvl_writer.emplace_child(lvl_name_hash(filepath.stem().string()))
         .write(reader.read_array<std::byte>(reader.size()));
   }
   else {
      writer.write(reader.read_array<std::byte>(reader.size()));
   }

   added_files.emplace(normalized_path);
}

void write_req_to_lvl(const fs::path& req_file_path, ucfb::Writer& writer,
                      std::unordered_set<fs::path::string_type>& added_files,
                      const std::vector<fs::path>& source_dirs,
                      const std::unordered_set<std::string>& extern_files,
                      std::vector<std::pair<std::string, std::vector<std::string>>> req_file_contents)
{
   for (auto& section : req_file_contents) {
      for (auto& value : section.second) {
         const auto filename = value + "."s + section.first;

         if (const auto path = find_file(filename, source_dirs); path) {
            write_file_to_lvl(req_file_path, writer, added_files, source_dirs,
                              extern_files, *path);
         }
         else if (!extern_files.count(boost::to_lower_copy(filename))) {
            synced_error_print("Warning nonexistent file "sv, std::quoted(filename),
                               " referenced in "sv, req_file_path, '.');
         }
      }
   }
}

void build_lvl_file(const fs::path& req_file_path, const fs::path& output_directory,
                    const std::vector<fs::path>& source_dirs,
                    const std::unordered_set<std::string>& extern_files) noexcept
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

      write_req_to_lvl(req_file_path, writer, added_files, source_dirs,
                       extern_files, parse_req_file(req_file_path));

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
   auto input_filter = R"(.+\.req)"s;
   std::vector<std::string> source_directories;
   std::vector<std::string> extern_files_list_paths;
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
      | Opt{input_filter, "input filter"s}
      ["--inputfilter"s]["-f"s]
      ("Regular Expression (EMCA Script syntax) to test each possible input filename " 
       " against. Default is \".+\\.req\""s)
      | Opt{extern_files_list_paths, "extern files list"s}
      ["--externfilelist"s]["-e"s]
      ("Path to a .req file listing external files no warning will be issued for "
       "files that can not be found listed in the specified file. Multiple files can be"
       " provided."s)
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

   if (!fs::exists(output_dir) && !fs::create_directories(output_dir)) {
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

   const std::vector<fs::path> source_directories_paths{std::cbegin(source_directories),
                                                        std::cend(source_directories)};

   for (const auto& path : source_directories_paths) {
      if (!fs::exists(path) || !fs::is_directory(path)) {
         synced_error_print("Warning source directory "sv, path, " does not exist!"sv);
      }
   }

   const auto extern_files = make_extern_files_set(extern_files_list_paths);

   const auto process_directory = [&](auto&& iterator) {
      for (auto& entry : std::forward<decltype(iterator)>(iterator)) {
         if (!fs::is_regular_file(entry.path())) continue;

         if (!std::regex_match(entry.path().filename().string(),
                               std::regex{input_filter, std::regex::ECMAScript})) {
            continue;
         }

         synced_print("Munging lvl "sv, entry.path().filename().string(), "..."sv);

         build_lvl_file(entry.path(), output_dir, source_directories_paths, extern_files);
      }
   };

   if (recursive) {
      process_directory(fs::recursive_directory_iterator{input_dir});
   }
   else {
      process_directory(fs::directory_iterator{input_dir});
   }
}
