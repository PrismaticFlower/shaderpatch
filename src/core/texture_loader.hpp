#pragma once

#include "texture_database.hpp"

#include <filesystem>
#include <utility>

struct ID3D11Device1;

namespace sp::core {

void load_texture_lvl(const std::filesystem::path lvl_path, ID3D11Device1& device,
                      Shader_resource_database& database) noexcept;

}
