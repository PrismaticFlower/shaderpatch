
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration normal_declaration{
   .rendertype = Rendertype::normal,

   .pool =
      std::array{Shader_entry{.name = "unlit opaque"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "unlit opaque hardedged"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "unlit transparent"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "unlit transparent hardedged"sv,

                              .skinned = true,
                              .lighting = false,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque hardedged"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent hardedged"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque hardedged projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent hardedged projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque shadow"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque hardedged shadow"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque shadow projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near opaque hardedged shadow projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent shadow"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent hardedged shadow"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent shadow projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}},

                 Shader_entry{.name = "near transparent hardedged shadow projectedtex"sv,

                              .skinned = true,
                              .lighting = true,
                              .vertex_color = true,
                              .texture_coords = true,
                              .base_input = Base_input::normals,

                              .srgb_state = {true, false, true, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "unlit opaque"sv})},

      Shader_state{.passes = single_pass({.shader_name = "unlit opaque hardedged"sv})},

      Shader_state{.passes = single_pass({.shader_name = "unlit transparent"sv})},

      Shader_state{.passes = single_pass({.shader_name = "unlit transparent hardedged"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near opaque"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near opaque hardedged"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near transparent"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near transparent hardedged"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near opaque projectedtex"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near opaque hardedged projectedtex"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near transparent projectedtex"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near transparent hardedged projectedtex"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near opaque shadow"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near opaque hardedged shadow"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near opaque shadow projectedtex"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near opaque hardedged shadow projectedtex"sv})},

      Shader_state{.passes = single_pass({.shader_name = "near transparent shadow"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near transparent hardedged shadow"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "near transparent shadow projectedtex"sv})},

      Shader_state{
         .passes = single_pass(
            {.shader_name = "near transparent hardedged shadow projectedtex"sv})}}};

}
