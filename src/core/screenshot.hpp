#pragma once

#include "swapchain.hpp"

#include <d3d11_2.h>

namespace sp::core {

void screenshot(ID3D11Device2& device, ID3D11DeviceContext2& dc,
                const Swapchain& swapchain,
                const std::filesystem::path& save_folder) noexcept;
}
