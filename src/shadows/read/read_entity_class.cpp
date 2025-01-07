#include "read_entity_class.hpp"

#include "swbf_fnv_1a.hpp"

#pragma warning(disable : 4063)

namespace sp::shadows {

auto read_entity_class(ucfb::File_reader_child& entc) -> Input_entity_class
{
   Input_entity_class result;

   while (entc) {
      auto child = entc.read_child();

      switch (child.magic_number()) {
      case "BASE"_mn: {
         result.base_name = child.read_string();
      } break;
      case "TYPE"_mn: {
         result.type_name = child.read_string();
      } break;
      case "PROP"_mn: {
         switch (child.read<std::uint32_t>()) {
         case "GeometryName"_fnv: {
            result.geometry_name = child.read_string();
         } break;
         }
      } break;
      }
   }

   return result;
}

}