
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration stencilshadow_declaration{
   .rendertype = Rendertype::stencilshadow,

   .pool =
      std::array{Shader_entry{.name = "cpu pre-extended"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend directional unskinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend directional hard skinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend directional hard skinned generate normal"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend directional soft skinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend directional soft skinned generate normal"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend point unskinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend point hard skinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend point hard skinned generate normal"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend point soft skinned"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}},

                 Shader_entry{.name = "extend point soft skinned generate normal"sv,

                              .skinned = false,
                              .lighting = false,
                              .vertex_color = false,
                              .texture_coords = false,
                              .base_input = Base_input::none,

                              .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "cpu pre-extended"sv})},

      Shader_state{.passes = single_pass({.shader_name = "extend directional unskinned"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend directional hard skinned"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend directional hard skinned generate normal"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend directional soft skinned"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend directional soft skinned generate normal"sv})},

      Shader_state{.passes = single_pass({.shader_name = "extend point unskinned"sv})},

      Shader_state{.passes = single_pass({.shader_name = "extend point hard skinned"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend point hard skinned generate normal"sv})},

      Shader_state{.passes = single_pass({.shader_name = "extend point soft skinned"sv})},

      Shader_state{.passes = single_pass(
                      {.shader_name = "extend point soft skinned generate normal"sv})},

      Shader_state{
         .passes = std::array{Shader_pass{.shader_name = "extend directional unskinned"sv},

                              Shader_pass{.shader_name = "extend point unskinned"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "extend directional hard skinned"sv},

                       Shader_pass{.shader_name = "extend point hard skinned"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "extend directional hard skinned generate normal"sv},

                       Shader_pass{.shader_name = "extend point hard skinned generate normal"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "extend directional soft skinned"sv},

                       Shader_pass{.shader_name = "extend point soft skinned"sv}}},

      Shader_state{
         .passes =
            std::array{Shader_pass{.shader_name = "extend directional soft skinned generate normal"sv},

                       Shader_pass{.shader_name =
                                      "extend point soft skinned generate normal"sv}}}}};

}
