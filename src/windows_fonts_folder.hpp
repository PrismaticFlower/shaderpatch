#pragma once

#include <filesystem>

namespace sp {

auto windows_fonts_folder() noexcept -> const std::filesystem::path&;

}
