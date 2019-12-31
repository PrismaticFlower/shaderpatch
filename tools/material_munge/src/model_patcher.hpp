#pragma once

#include "material_options.hpp"
#include "string_utilities.hpp"

#include <filesystem>
#include <unordered_map>

namespace sp {

void patch_model(const std::filesystem::path& model_path,
                 const std::filesystem::path& output_model_path,
                 const std::unordered_map<Ci_string, Material_options>& material_index,
                 const bool patch_material_flags);

}
