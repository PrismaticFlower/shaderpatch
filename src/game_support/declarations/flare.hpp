
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration flare_declaration{
   .rendertype = Rendertype::flare,

   .pool = std::array{Shader_entry{.name = "textured flare"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "untextured flare"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = false,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "textured flare"sv})},

                 Shader_state{
                    .passes = single_pass({.shader_name = "untextured flare"sv})}}};

}
