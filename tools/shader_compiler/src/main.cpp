
#include "game_compiler.hpp"

#include <iostream>
#include <stdexcept>

int main(int arg_count, char* args[])
{
   if (arg_count != 4) {
      std::cout << "usage: <definition path> <hlsl path> <out path>\n";

      return 1;
   }

   try {
      sp::Game_compiler{args[1], args[2]}.save(args[3]);
   }
   catch (std::exception& e) {
      std::cerr << e.what() << '\n';

      return 1;
   }
}
