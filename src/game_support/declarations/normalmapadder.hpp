
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration normalmapadder_declaration{
   .rendertype = Rendertype::normalmapadder,

   .pool = std::array{Shader_entry{.name = "discard"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "discard tangents"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::tangents,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "discard"sv})},

                 Shader_state{
                    .passes = single_pass({.shader_name = "discard tangents"sv})}}};

}
