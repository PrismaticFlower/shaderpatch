
#include "game_compiler.hpp"
#include "patch_compiler.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <clara.hpp>

using namespace std::literals;

int main(int arg_count, char* args[])
{
   using namespace clara;

   bool help = false;
   bool patchshader = false;
   std::string def_file;
   std::string output_file;
   std::string source_file;

   // clang-format off

   auto cli = Help(help) 
            | Opt(patchshader)
                 ["-p"]["--patchshader"]
                 ("Compile shader for consumption by Shader Patch.")
            | Arg(def_file, "definition file"s)
                 ("Path to the JSON file defining the shader."s)
            | Arg(output_file, "output file"s)
                 ("Output file path."s)
            | Arg(source_file, "source file"s)
                 ("Path to the source file for game shaders."s);

   // clang-format on

   const auto result = cli.parse(Args(arg_count, args));

   if (!result) {
      std::cerr << "Commandline Error: "sv << result.errorMessage() << '\n';

      return 1;
   }
   else if (help) {
      std::cout << cli;

      return 0;
   }

   try {
      if (!patchshader) {
         sp::Game_compiler{def_file, source_file}.save(output_file);
      }
      else {
         sp::Patch_compiler compiler{def_file};

         compiler.optimize_permutations();

         compiler.save(output_file);
      }
   }
   catch (std::exception& e) {
      std::cerr << e.what() << '\n';

      return 1;
   }
}