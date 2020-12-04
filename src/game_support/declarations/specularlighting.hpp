
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration specularlighting_declaration{
   .rendertype = Rendertype::specularlighting,

   .pool = std::array{Shader_entry{.name = "vertex lit specular"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "specular 3 lights"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "specular 2 lights"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "specular 1 lights"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "specular normalmapped"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::tangents,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "normalmapped reflection"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::tangents,

                                   .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "vertex lit specular"sv})},

      Shader_state{.passes = single_pass({.shader_name = "specular 3 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "specular 2 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "specular 1 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "specular normalmapped"sv})},

      Shader_state{.passes = single_pass({.shader_name = "normalmapped reflection"sv})},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "specular normalmapped"sv},

                       Shader_pass{.shader_name = "normalmapped reflection"sv}}}}};

}
