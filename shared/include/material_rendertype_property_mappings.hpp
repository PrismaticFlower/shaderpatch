#pragma once

#include "patch_material_io.hpp"

#include <utility>
#include <vector>

#include <absl/container/flat_hash_map.h>

namespace sp {

inline const absl::flat_hash_map<std::string, std::vector<std::pair<std::string, Material_property>>> rendertype_property_mappings{
   {"normal_ext",
    {
       {"specular", {"UseSpecularLighting", true}},
       {"dynamic", {"IsDynamic", true}},
    }},

   {"normal_ext_terrain",
    {
       {"parallax offset mapping", {"DisplacementMode", 1}},
       {"parallax occlusion mapping", {"DisplacementMode", 2}},
       {"basic blending", {"BlendMode", 1}},
       {"low detail", {"LowDetail", true}},
    }},

   {"pbr",
    {
       {"basic", {"NoMetallicRoughnessMap", true}},
       {"emissive", {"UseEmissiveMap", true}},
       // TODO: Remove this TEMP one.
       {"TEMP ibl", {"TEMP_UseIBL", true}},
    }},

   {"pbr_terrain",
    {
       {"parallax offset mapping", {"DisplacementMode", 1}},
       {"parallax occlusion mapping", {"DisplacementMode", 2}},
       {"basic blending", {"BlendMode", 1}},
       {"low detail", {"LowDetail", true}},
    }},

   {"skybox",
    {
       {"emissive", {"UseEmissiveMap", true}},
    }},

   {"static_water",
    {
       {"specular", {"UseSpecularLighting", true}},
    }},
};

}
