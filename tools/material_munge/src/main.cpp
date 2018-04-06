
#include "helpers.hpp"
#include "munge_materials.hpp"
#include "synced_io.hpp"

#include <future>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <clara.hpp>

using namespace std::literals;

using namespace sp;

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   auto source_dir = "./"s;
   auto munged_input_dir = "./"s;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place output files."s)
      | Opt{source_dir, "source directory"s}
      ["--sourcedir"s]
      ("Path to input .mtrl files."s)
      | Opt{munged_input_dir, "munged source directory"s}
      ["--mungedsourcedir"s]
      ("Path to input munged files."s);

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

   auto files_result =
      std::async(std::launch::async, build_input_file_map, source_dir);

   auto texture_references = find_texture_references(munged_input_dir);

   auto files = files_result.get();

   munge_materials(output_dir, texture_references, files);

   return 0;
}
