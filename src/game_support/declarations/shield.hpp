
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration shield_declaration{
   .rendertype = Rendertype::shield,

   .pool = std::array{Shader_entry{.name = "shield"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {true, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "shield"sv})}}};

}
