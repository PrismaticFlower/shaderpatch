#pragma once

#include "framework.hpp"

auto load_xml_install_manifest(const std::filesystem::path& base_path)
   -> std::vector<std::filesystem::path>;
