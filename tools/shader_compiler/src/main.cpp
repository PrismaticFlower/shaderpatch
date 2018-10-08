
#include "compiler_helpers.hpp"
#include "declaration_munge.hpp"
#include "patch_compiler.hpp"
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
   auto def_file_dir = "./"s;
   auto source_file_dir = "./"s;
   auto output_dir = "./"s;
   DWORD optimization_flag = 0;
   bool debug_info = false;
   bool fatal_warnings = false;

   const auto parse_optimization_level = [&](int i) {
      switch (i) {
      case 0:
         optimization_flag = D3DCOMPILE_OPTIMIZATION_LEVEL0;
         break;
      case 1:
         optimization_flag = D3DCOMPILE_OPTIMIZATION_LEVEL1;
         break;
      case 2:
         optimization_flag = D3DCOMPILE_OPTIMIZATION_LEVEL2;
         break;
      case 3:
         optimization_flag = D3DCOMPILE_OPTIMIZATION_LEVEL3;
         break;
      default:
         return ParserResult::runtimeError(
            "optimization level must be 0, 1, 2 or 3.");
      }

      return ParserResult::ok(ParseResultType::Matched);
   };

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place munged files in."s)
      | Opt{decl_file_dir, "shader declarations directory"s}
      ["--declarationinputdir"s]
      | Opt{def_file_dir, "shader definitions directory"s}
      ["--definitioninputdir"s]
      ("Input directory for shader definition files."s)
      | Opt{source_file_dir, "hlsl source directory"s}
      ["--hlslinputdir"s]
      ("Input directory for HLSL source files."s) 
      | Opt{parse_optimization_level, "optimization level"s}
      ["-o"]["--optimizationlevel"]
      ("Optimization level passed to D3DCompile."s)
      | Opt{debug_info, "include debug info"s}
      ["-d"]["--debuginfo"]
      ("Include debug info in compiled shaders."s)
      | Opt{fatal_warnings, "fatal warnings"s}
      ["-f"]["--fatalwarnings"]
      ("Warnings will be treated as errors."s);

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

   fs::path source_file_dir_path = source_file_dir;
   fs::path output_dir_path = output_dir;

   if (!fs::exists(decl_file_dir)) {
      synced_error_print("Declaration Source Directory "sv,
                         std::quoted(def_file_dir), " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(def_file_dir)) {
      synced_error_print("Definition Source Directory "sv,
                         std::quoted(def_file_dir), " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(source_file_dir_path)) {
      synced_error_print("HLSL Source Directory "sv, source_file_dir_path,
                         " does not exist!"sv);

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

   // Compile Shaders
   std::vector<fs::path> definition_files;
   definition_files.reserve(64);

   for (auto& entry : fs::recursive_directory_iterator{def_file_dir}) {
      if (!fs::is_regular_file(entry.path())) continue;
      if (entry.path().extension() != ".json"s) continue;

      definition_files.emplace_back(entry.path());
   }

   auto compiler_flags = optimization_flag;

   if (debug_info) compiler_flags |= D3DCOMPILE_DEBUG;
   if (fatal_warnings) compiler_flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

   auto predicate = [&](const fs::path& path) {
      try {
         auto definition = read_json_file(path);

         sp::Patch_compiler{compiler_flags, definition, path,
                            source_file_dir_path, output_dir_path};
      }
      catch (std::exception& e) {
         synced_error_print("Error occured while munging "sv,
                            path.filename().string(), " message:\n"sv, e.what());
      }
   };

   std::for_each(std::execution::par, std::cbegin(definition_files),
                 std::cend(definition_files), predicate);
}
