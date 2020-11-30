
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration water_declaration{
   .rendertype = Rendertype::water,

   .pool =
      std::array{Shader_entry{.name = "discard"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::position,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "projective bumpmap with distorted reflection with specular"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "projective bumpmap with distorted reflection"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "projective bumpmap without distortion with specular"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normal map without distortion"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normal map with reflection with specular"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normal map with reflection"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "transmissive fade"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::position,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normal map without reflection with specular"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normal map without reflection"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "low quality with specular"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, true, false, false}},

                 Shader_entry{.name = "low quality"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, true, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "projective bumpmap with distorted reflection with specular"sv}}},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "projective bumpmap with distorted reflection"sv}}},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "projective bumpmap without distortion with specular"sv}}},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "normal map without distortion"sv}}},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "normal map with reflection with specular"sv}}},

      Shader_state{.passes = std::array{Shader_pass{.shader_name = "discard"sv},

                                        Shader_pass{.shader_name = "normal map with reflection"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "transmissive fade"sv},

                              Shader_pass{.shader_name = "normal map without reflection with specular"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "transmissive fade"sv},

                              Shader_pass{.shader_name = "normal map without reflection"sv}}},

      Shader_state{.passes = single_pass({.shader_name = "low quality with specular"sv})},

      Shader_state{.passes = single_pass({.shader_name = "low quality"sv})}}};

}
