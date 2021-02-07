
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration zprepass_declaration{
   .rendertype = Rendertype::zprepass,

   .pool = std::array{Shader_entry{.name = "near opaque"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "near opaque hardedged"sv,

                                   .skinned = true,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = true,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "near opaque"sv})},

                 Shader_state{.passes = single_pass(
                                 {.shader_name = "near opaque hardedged"sv})}}};

}
