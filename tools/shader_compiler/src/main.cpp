
#include "compiler_helpers.hpp"
#include "game_compiler.hpp"
#include "patch_compiler.hpp"
#include "synced_io.hpp"

#include <execution>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <boost/filesystem.hpp>
#include <clara.hpp>

using namespace std::literals;
using namespace sp;

namespace fs = boost::filesystem;

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   auto def_file_dir = "./"s;
   auto source_file_dir = "./"s;
   auto output_dir = "./"s;

   // clang-format off

   auto cli = Help{help}
      | Opt{output_dir, "output directory"s}
      ["--outputdir"s]
      ("Path to place munged files in."s)
      | Opt{def_file_dir, "shader definitions directory"s}
      ["--definitioninputdir"s]
      ("Input directory for shader definition files."s)
      | Opt{source_file_dir, "hlsl source directory"s}
      ["--hlslinputdir"s]
      ("Input directory for HLSL source files."s);

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

   if (!fs::exists(def_file_dir)) {
      synced_error_print("Source Directory "sv, std::quoted(def_file_dir),
                         " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(source_file_dir_path)) {
      synced_error_print("Source Directory "sv, source_file_dir_path,
                         " does not exist!"sv);

      return 1;
   }

   if (!fs::exists(output_dir_path) && !fs::create_directories(output_dir_path)) {
      synced_error_print("Unable to create output directory "sv,
                         std::quoted(output_dir), "."sv);

      return 1;
   }

   std::vector<fs::path> definition_files;
   definition_files.reserve(64);

   for (auto& entry : fs::recursive_directory_iterator{def_file_dir}) {
      if (!fs::is_regular_file(entry.path())) continue;
      if (entry.path().extension() != ".json"s) continue;

      definition_files.emplace_back(entry.path());
   }

   auto predicate = [&](const fs::path& path) {
      try {
         auto definition = read_definition_file(path);

         if (definition.value("patch_shader"s, false)) {
            sp::Patch_compiler{definition, path, source_file_dir_path, output_dir_path};
         }
         else {
            sp::Game_compiler{definition, path, source_file_dir_path, output_dir_path};
         }
      }
      catch (std::exception& e) {
         synced_error_print("Error occured while munging "sv,
                            path.filename().string(), " message:\n"sv, e.what());
      }
   };

   std::for_each(std::execution::par, std::cbegin(definition_files),
                 std::cend(definition_files), predicate);
}
