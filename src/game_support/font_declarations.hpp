#pragma once

#include "ucfb_editor.hpp"

#include <array>

namespace sp::game_support {

const std::size_t font_count = 5;

using Font_declarations =
   std::array<std::pair<std::string_view, ucfb::Editor_data_chunk>, font_count>;

auto create_font_declarations(const std::filesystem::path& font_path)
   -> Font_declarations;

}
