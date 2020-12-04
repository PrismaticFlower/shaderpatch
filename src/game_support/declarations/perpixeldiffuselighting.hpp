
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration perpixeldiffuselighting_declaration{
   .rendertype = Rendertype::perpixeldiffuselighting,

   .pool =
      std::array{Shader_entry{.name = "perpixel 3 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "perpixel 2 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "perpixel 1 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "perpixel spotlight"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 3 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::tangents,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 2 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::tangents,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 1 lights"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::tangents,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped spotlight"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::tangents,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 3 lights (generate tangents & texcoords)"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = false,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 2 lights (generate tangents & texcoords)"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = false,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped 1 lights (generate tangents & texcoords)"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = false,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "normalmapped spotlight (generate tangents & texcoords)"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = false,
                              .base_input = Base_input::normals,

                              .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "perpixel 3 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "perpixel 2 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "perpixel 1 lights"sv})},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 3 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 2 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 1 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{.passes = single_pass({.shader_name = "normalmapped 3 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "normalmapped 2 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "normalmapped 1 lights"sv})},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "normalmapped 3 lights"sv},

                              Shader_pass{.shader_name = "normalmapped spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "normalmapped 2 lights"sv},

                              Shader_pass{.shader_name = "normalmapped spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "normalmapped 1 lights"sv},

                              Shader_pass{.shader_name = "normalmapped spotlight"sv}}},

      Shader_state{.passes = single_pass({.shader_name = "perpixel 3 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "perpixel 2 lights"sv})},

      Shader_state{.passes = single_pass({.shader_name = "perpixel 1 lights"sv})},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 3 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 2 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "perpixel 1 lights"sv},

                              Shader_pass{.shader_name = "perpixel spotlight"sv}}},

      Shader_state{.passes = single_pass(
                      {.shader_name = "normalmapped 3 lights (generate tangents & texcoords)"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "normalmapped 2 lights (generate tangents & texcoords)"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "normalmapped 1 lights (generate tangents & texcoords)"sv})},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "normalmapped 3 lights (generate tangents & texcoords)"sv},

                       Shader_pass{.shader_name = "normalmapped spotlight (generate tangents & texcoords)"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "normalmapped 2 lights (generate tangents & texcoords)"sv},

                       Shader_pass{.shader_name = "normalmapped spotlight (generate tangents & texcoords)"sv}}},

      Shader_state{
         .passes = std::array{
            Shader_pass{.shader_name = "normalmapped 1 lights (generate tangents & texcoords)"sv},

            Shader_pass{.shader_name =
                           "normalmapped spotlight (generate tangents & texcoords)"sv}}}}};

}
