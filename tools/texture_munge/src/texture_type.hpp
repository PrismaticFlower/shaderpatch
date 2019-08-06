#pragma once

#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <string_view>

namespace sp {
enum class Texture_type {
   passthrough,
   image,
   roughness,
   metellic_roughness,
   normal_map,
   cubemap
};

inline auto texture_type_from_string(const std::string_view type) -> Texture_type
{
   if (type == "basic"_svci) {
      synced_print("Found texture with type \"basic\", make sure you do not "
                   "want to take advantage of the new types, and then switch "
                   "to \"image\".");

      return Texture_type::image;
   }
   else if (type == "passthrough"_svci) {
      return Texture_type::passthrough;
   }
   else if (type == "image"_svci) {
      return Texture_type::image;
   }
   else if (type == "roughness"_svci) {
      return Texture_type::roughness;
   }
   else if (type == "metellicroughness"_svci) {
      return Texture_type::metellic_roughness;
   }
   else if (type == "normalmap"_svci) {
      return Texture_type::normal_map;
   }
   else if (type == "cubemap"_svci) {
      return Texture_type::cubemap;
   }

   throw std::invalid_argument{"Invalid texture type"};
}

}
