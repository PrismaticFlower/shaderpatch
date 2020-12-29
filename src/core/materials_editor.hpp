#pragma once

#include "../material/material.hpp"
#include "texture_database.hpp"

namespace sp::core {

void show_materials_editor(
   ID3D11Device5& device, Shader_resource_database& resources,
   const std::vector<std::unique_ptr<material::Material>>& materials) noexcept;
}
