#pragma once

#include "../texture.hpp"
#include "postprocess.hpp"

#include <array>

namespace sp::effects {

glm::vec3 get_lut_exposure_color_filter(const Color_grading_params& params) noexcept;

auto bake_color_grading_luts(IDirect3DDevice9& device,
                             const Color_grading_params& params) noexcept
   -> std::array<Texture, 3>;

}
