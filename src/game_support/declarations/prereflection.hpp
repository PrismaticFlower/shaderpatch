
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration prereflection_declaration{
   .rendertype = Rendertype::prereflection,

   .pool = std::array{Shader_entry{.name = "prereflection"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "prereflection fake stencil"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "prereflection"sv})},

                 Shader_state{.passes = single_pass(
                                 {.shader_name = "prereflection fake stencil"sv})}}};

}
