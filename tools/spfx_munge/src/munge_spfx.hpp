#pragma once

#include <filesystem>

namespace sp {

void munge_spfx(const std::filesystem::path& spfx_path,
                const std::filesystem::path& output_dir);

}
