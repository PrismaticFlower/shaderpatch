#pragma once

namespace sp::game_support {

constexpr int aspect_ratio_device_ptr_offset = -7;

void set_aspect_ratio_ptr(float* aspect_ratio_ptr) noexcept;

void set_aspect_ratio(float new_aspect_ratio) noexcept;

[[nodiscard]] auto get_aspect_ratio() noexcept -> float;

}
