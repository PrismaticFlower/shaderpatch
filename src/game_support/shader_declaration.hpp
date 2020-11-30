#pragma once

#include "game_rendertypes.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <absl/container/fixed_array.h>

namespace sp::game_support {

enum class Base_input { none, position, normals, tangents };

struct Shader_entry {
   std::string_view name;

   bool skinned = false;
   bool lighting = false;
   bool vertex_color = false;
   bool texture_coords = false;
   Base_input base_input = Base_input::none;

   std::array<bool, 4> srgb_state;
};

struct Shader_pass {
   std::string_view shader_name;
};

template<std::size_t pass_count>
struct Shader_state {
   std::array<Shader_pass, pass_count> passes;
};

template<std::size_t shader_pool_size, std::size_t... state_pass_counts>
struct Shader_declaration {
   Rendertype rendertype{};

   std::array<Shader_entry, shader_pool_size> pool;
   std::tuple<Shader_state<state_pass_counts>...> states;
};

constexpr auto single_pass(Shader_pass pass) noexcept -> std::array<Shader_pass, 1>
{
   return {pass};
}

}
