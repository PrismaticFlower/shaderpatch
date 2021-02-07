#pragma once

#include "../core/texture_database.hpp"
#include "factory.hpp"
#include "material.hpp"

#include <memory>
#include <vector>

namespace sp::material {

void show_editor(Factory& factory,
                 const std::vector<std::unique_ptr<Material>>& materials) noexcept;
}
