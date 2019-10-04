#pragma once

#include "texture_database.hpp"

#include <d3d11_4.h>

namespace sp::core {

void add_builtin_textures(ID3D11Device5& device,
                          Shader_resource_database& resources) noexcept;

}