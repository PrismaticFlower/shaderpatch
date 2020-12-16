#pragma once

#include "framework.hpp"

auto load_install_manifest(const std::filesystem::path& base_path) noexcept
   -> std::vector<std::filesystem::path>;

void save_install_manifest(const std::filesystem::path& base_path,
                           const std::vector<std::filesystem::path>& paths) noexcept;
