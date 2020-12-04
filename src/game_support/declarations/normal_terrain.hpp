
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration normal_terrain_declaration{
   .rendertype = Rendertype::Terrain2,

   .pool = std::array{Shader_entry{.name = "diffuse blendmap"sv,

                                   .skinned = false,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = false,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {true, true, true, false}},

                      Shader_entry{.name = "diffuse blendmap unlit"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = false,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {true, true, true, false}},

                      Shader_entry{.name = "detailing"sv,

                                   .skinned = false,
                                   .lighting = true,
                                   .vertex_color = true,
                                   .texture_coords = false,
                                   .base_input = Base_input::normals,

                                   .srgb_state = {false, false, true, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "diffuse blendmap"sv})},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "diffuse blendmap unlit"sv},

                                        Shader_pass{.shader_name = "detailing"sv}}}}};

}
