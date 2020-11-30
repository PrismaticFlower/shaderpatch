
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration rain_declaration{
   .rendertype = Rendertype::rain,

   .pool = std::array{Shader_entry{.name = "rain"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = true,
                                   .texture_coords = false,
                                   .base_input = Base_input::position,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "rain"sv})}}};

}
