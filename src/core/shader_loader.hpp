#pragma once

#include "input_layout_factory.hpp"
#include "shader_database.hpp"

#include <filesystem>
#include <utility>

struct ID3D11Device;

namespace sp::core {

auto load_shader_lvl(const std::filesystem::path lvl_path, ID3D11Device& device,
                     Input_layout_factory& input_layout_factory) noexcept
   -> Shader_database;
}
