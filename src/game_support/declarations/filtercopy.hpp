
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration filtercopy_declaration{
   .rendertype = Rendertype::filtercopy,

   .pool = std::array{Shader_entry{.name = "filtercopy four textures"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "filtercopy one texture"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "filtercopy two textures"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "filtercopy three textures"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "filtercopy four textures"sv})},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "filtercopy four textures"sv},

                              Shader_pass{.shader_name = "filtercopy one texture"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "filtercopy four textures"sv},

                              Shader_pass{.shader_name = "filtercopy two textures"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "filtercopy four textures"sv},

                       Shader_pass{.shader_name = "filtercopy three textures"sv}}}}};

}
