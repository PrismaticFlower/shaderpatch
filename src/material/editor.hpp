#pragma once

#include "../core/texture_database.hpp"
#include "material.hpp"

namespace sp::material {

void show_editor(ID3D11Device5& device, core::Shader_resource_database& resources,
                 const std::vector<std::unique_ptr<Material>>& materials) noexcept;
}
