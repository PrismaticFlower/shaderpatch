
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration ocean_declaration{
   .rendertype = Rendertype::ocean,

   .pool = std::array{Shader_entry{.name = "far ocean"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {true, true, false, false}},

                      Shader_entry{.name = "near ocean"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {true, true, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "far ocean"sv})},

                 Shader_state{.passes = single_pass({.shader_name = "near ocean"sv})}}};

}
