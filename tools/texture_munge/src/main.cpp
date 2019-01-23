
#include "munge_texture.hpp"
#include "synced_io.hpp"

#include <filesystem>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>

#include <Compressonator.h>
#include <Windows.h>
#include <clara.hpp>

using namespace std::literals;
using namespace sp;

namespace fs = std::filesystem;

int main(int arg_count, char* args[])
{
   CoInitializeEx(nullptr, COINIT_MULTITHREADED);
   CMP_InitializeBCLibrary();

   using namespace clara;

   bool help = false;
   auto output_dir = "./"s;
   auto source_dir = "./"s;
   auto input_filter = R"(.+\.tex)"s;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]["-o"s]
      ("Path to place munged files in."s)
      | Opt{source_dir, "source directory"s}
      ["--sourcedir"s]["-s"s]
      ("Path to search for input texture files."s)
      | Opt{input_filter, "input filter"s}
      ["--inputfilter"s]["-f"s]
      ("Regular Expression (EMCA Script syntax) Filter to test files in the source "
       "directory against. Any file that passes will be considered a YAML config file "
       " for a texture. Default is \".+\\.tex\""s);

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

   for (auto& entry : fs::recursive_directory_iterator{source_dir}) {
      if (!fs::is_regular_file(entry.path())) continue;

      if (!std::regex_match(entry.path().string(),
                            std::regex{input_filter, std::regex::ECMAScript})) {
         continue;
      }

      munge_texture(entry.path(), output_dir);
   }
}
