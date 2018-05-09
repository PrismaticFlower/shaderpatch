#pragma once

#include <filesystem>

namespace sp {

void edit_envfx(const std::filesystem::path& fx_path,
                const std::filesystem::path& output_dir);

}
