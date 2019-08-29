#pragma once

#include <filesystem>

namespace sp {

void munge_colorgrading_regions(const std::filesystem::path& regions_path,
                                const std::filesystem::path& config_search_path,
                                const std::filesystem::path& output_path) noexcept;

}