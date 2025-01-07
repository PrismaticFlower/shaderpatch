#include "read_game_model.hpp"

#pragma warning(disable : 4063)

namespace sp::shadows {

auto read_game_model(ucfb::File_reader_child& gmod) -> Input_game_model
{
   Input_game_model result;

   while (gmod) {
      auto child = gmod.read_child();

      switch (child.magic_number()) {
      case "NAME"_mn: {
         result.name = child.read_string();
      } break;
      case "INFO"_mn: {
         // Unsure if we need to care about LOD bias and such.
      } break;
      case "LOD0"_mn: {
         result.lod0 = child.read_string();
         result.lod0_tris = child.read<std::uint32_t>();
      } break;
      case "LOD1"_mn: {
         result.lod1 = child.read_string();
         result.lod1_tris = child.read<std::uint32_t>();
      } break;
      case "LOD2"_mn: {
         result.lod2 = child.read_string();
         result.lod2_tris = child.read<std::uint32_t>();
      } break;
      case "LOWD"_mn: {
         result.lowd = child.read_string();
         result.lowd_tris = child.read<std::uint32_t>();
      } break;
      }
   }

   return result;
}

}