
#include "helpers.hpp"
#include "munge_helpers.hpp"
#include "munge_materials.hpp"
#include "synced_io.hpp"

#include <filesystem>
#include <future>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <clara.hpp>

using namespace std::literals;
using namespace sp;

namespace fs = std::filesystem;

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   auto source_dir = "./"s;
   auto munged_input_dir = "./"s;
   std::vector<std::string> description_dirs;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]["-o"s]
      ("Path to place munged files in."s)
      | Opt{source_dir, "source directory"s}
      ["--sourcedir"s]["-s"s]
      ("Path to search for input .mtrl files."s)
      | Opt{munged_input_dir, "munged source directory"s}
      ["--mungedsourcedir"s]["-m"s]
      ("Path to input munged files."s)
      | Opt{description_dirs, "description directory"s}
      ["--descdir"s]["-d"s]
      ("Add a path to search (non recursively) for input *.yml files"
       " holding descriptions of rendertypes."s);

   // clang-format on

   const auto result = cli.parse(Args{arg_count, args});

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
                         std::quoted(output_dir), "."sv);

      return 1;
   }

   if (!fs::exists(source_dir)) {
      synced_error_print("Source Directory "sv, std::quoted(source_dir),
                         " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(munged_input_dir)) {
      synced_error_print("Munged Input Directory "sv,
                         std::quoted(munged_input_dir), " does not exist!"sv);

      return 1;
   }

   auto files_result =
      std::async(std::launch::deferred, build_input_file_map, source_dir);

   auto descriptions_async =
      std::async(std::launch::deferred, load_material_descriptions, description_dirs);

   auto texture_references = find_texture_references(munged_input_dir);

   auto files = files_result.get();
   auto descriptions = descriptions_async.get();

   munge_materials(output_dir, texture_references, files, descriptions);

   return 0;
}
