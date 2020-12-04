#pragma once

#include "shader_metadata.hpp"
#include "ucfb_editor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sp::game_support {

struct Munged_shader_declarations {
   Munged_shader_declarations() noexcept = default;

   Munged_shader_declarations(const Munged_shader_declarations&) noexcept = delete;
   auto operator=(const Munged_shader_declarations&) noexcept
      -> Munged_shader_declarations& = delete;

   Munged_shader_declarations(Munged_shader_declarations&&) noexcept = default;
   auto operator=(Munged_shader_declarations&&) noexcept
      -> Munged_shader_declarations& = default;

   std::vector<Shader_metadata> shader_pool;

   std::array<ucfb::Editor_parent_chunk, 23> munged_declarations;
};

auto munged_shader_declarations() noexcept -> const Munged_shader_declarations&;

}
