#pragma once

#include "patch_material.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto describe_material(std::string_view name, const YAML::Node description,
                       const YAML::Node material) -> Material_info;
}
