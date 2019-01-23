#pragma once

#include <filesystem>
#include <string>

namespace sp {

auto load_string_file(const std::filesystem::path& path) -> std::string;

}
