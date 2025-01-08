#include "read_world.hpp"

#include "../shadow_world.hpp"

#include "swbf_fnv_1a.hpp"

#pragma warning(disable : 4063)

namespace sp::shadows {

namespace {

auto read_instance(ucfb::File_reader_child& inst, const std::string& layer_name,
                   bool in_child_lvl) -> Input_object_instance
{
   Input_object_instance result{
      .layer_name = layer_name,
      .in_child_lvl = in_child_lvl,
   };

   while (inst) {
      auto inst_child = inst.read_child();

      switch (inst_child.magic_number()) {
      case "INFO"_mn: {
         while (inst_child) {
            auto info_child = inst_child.read_child();

            switch (info_child.magic_number()) {
            case "TYPE"_mn: {
               result.type_name = info_child.read_string();
            } break;
            case "NAME"_mn: {
               result.name = info_child.read_string();
            } break;
            case "XFRM"_mn: {
               result.rotation = info_child.read<std::array<glm::vec3, 3>>();
               result.positionWS = info_child.read<glm::vec3>();
            } break;
            }
         }
      } break;
      case "PROP"_mn: {
         switch (inst_child.read<std::uint32_t>()) {
         case "GeometryName"_fnv: {
            result.geometry_name_override = inst_child.read_string();
         } break;
         }
      } break;
      }
   }

   return result;
}

}

void read_world(ucfb::File_reader_child& wrld, bool in_child_lvl)
{
   std::string name;

   while (wrld) {
      auto child = wrld.read_child();

      switch (child.magic_number()) {
      case "NAME"_mn: {
         name = child.read_string();
      } break;
      case "inst"_mn: {
         shadow_world.add_object_instance(read_instance(child, name, in_child_lvl));
      } break;
      }
   }
}
}