
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration particle_declaration{
   .rendertype = Rendertype::particle,

   .pool = std::array{Shader_entry{.name = "normal"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::position,

                                   .srgb_state = {true, false, false, false}},

                      Shader_entry{.name = "blur"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "normal"sv})},

                 Shader_state{.passes = single_pass({.shader_name = "blur"sv})}}};

}
