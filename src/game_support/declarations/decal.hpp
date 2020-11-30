
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration decal_declaration{
   .rendertype = Rendertype::decal,

   .pool = std::array{Shader_entry{.name = "diffuse"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "diffuse bump"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "bump"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {true, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "diffuse"sv})},

                 Shader_state{.passes = single_pass({.shader_name = "diffuse bump"sv})},

                 Shader_state{.passes = single_pass({.shader_name = "bump"sv})}}};

}
