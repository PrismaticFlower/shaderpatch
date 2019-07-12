#pragma once

#include <filesystem>

namespace sp {

void terrain_modelify(const std::filesystem::path& input_path,
                      const std::filesystem::path& output_path);

}
