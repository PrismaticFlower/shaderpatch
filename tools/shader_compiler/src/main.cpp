
#include "compiler_helpers.hpp"
#include "declaration_munge.hpp"
#include "synced_io.hpp"

#include <execution>
#include <filesystem>
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
   auto decl_file_dir = "./"s;
   auto output_dir = "./"s;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place munged files in."s)
      | Opt{decl_file_dir, "shader declarations directory"s}
      ["--declarationinputdir"s];

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

   fs::path output_dir_path = output_dir;

   if (!fs::exists(decl_file_dir)) {
      synced_error_print("Declaration Source Directory "sv,
                         std::quoted(decl_file_dir), " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(output_dir_path) && !fs::create_directories(output_dir_path)) {
      synced_error_print("Unable to create output directory "sv,
                         std::quoted(output_dir), "."sv);

      return 1;
   }

   // Munge Declarations
   for (auto& entry : fs::recursive_directory_iterator{decl_file_dir}) {
      if (!fs::is_regular_file(entry.path())) continue;
      if (entry.path().extension() != ".json"s) continue;

      Declaration_munge{entry.path(), output_dir_path};
   }
}
