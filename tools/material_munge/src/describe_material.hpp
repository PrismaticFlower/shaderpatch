#pragma once

#include "material_options.hpp"
#include "patch_material_io.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

auto describe_material(const std::string_view name,
                       const YAML::Node& description, const YAML::Node& material,
                       Material_options& material_options) -> Material_info;
}
