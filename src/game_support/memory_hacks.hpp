#pragma once

namespace sp::game_support {

void set_aspect_ratio_search_ptr(float* aspect_ratio_search_ptr) noexcept;

void find_aspect_ratio(float expected_aspect_ratio);

void set_aspect_ratio(float new_aspect_ratio) noexcept;

[[nodiscard]] auto get_aspect_ratio() noexcept -> float;

}
