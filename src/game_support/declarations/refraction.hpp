
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration refraction_declaration{
   .rendertype = Rendertype::refraction,

   .pool = std::array{Shader_entry{.name = "far"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}},

                      Shader_entry{.name = "nodistortion"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}},

                      Shader_entry{.name = "near"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}},

                      Shader_entry{.name = "near bump"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}},

                      Shader_entry{.name = "near diffuse"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}},

                      Shader_entry{.name = "near diffuse bump"sv,

                                   .skinned = true,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, true}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "far"sv})},

      Shader_state{.passes = single_pass({.shader_name = "nodistortion"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near bump"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near diffuse"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near diffuse bump"sv})}}};

}
