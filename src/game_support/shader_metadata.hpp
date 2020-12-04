#pragma once

#include "../shader/common.hpp"
#include "game_rendertypes.hpp"

#include <array>
#include <string_view>

namespace sp::game_support {

struct Shader_metadata {
   Rendertype rendertype{};
   std::string_view shader_name;

   shader::Vertex_shader_flags vertex_shader_flags = shader::Vertex_shader_flags::none;

   bool light_active = false;
   std::uint8_t light_active_point_count = 0;
   bool light_active_spot = 0;

   std::array<bool, 4> srgb_state = {false, false, false, false};
};

}
