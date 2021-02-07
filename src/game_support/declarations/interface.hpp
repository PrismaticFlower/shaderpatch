
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration interface_declaration{
   .rendertype = Rendertype::_interface,

   .pool = std::array{Shader_entry{.name = "masked bitmap element"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "vector element"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "untextured bitmap element"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "textured bitmap element"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "masked bitmap element"sv})},

      Shader_state{.passes = single_pass({.shader_name = "vector element"sv})},

      Shader_state{.passes = single_pass({.shader_name = "untextured bitmap element"sv})},

      Shader_state{.passes = single_pass({.shader_name = "textured bitmap element"sv})}}};

}
