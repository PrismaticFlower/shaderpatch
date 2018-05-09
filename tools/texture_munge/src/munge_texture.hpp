#pragma once

#include <filesystem>

namespace sp {

void munge_texture(std::filesystem::path config_file_path,
                   const std::filesystem::path& output_dir) noexcept;
}
