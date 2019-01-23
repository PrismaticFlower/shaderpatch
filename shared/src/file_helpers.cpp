#pragma once

#include "file_helpers.hpp"
#include "compose_exception.hpp"

#include <fstream>

#include <gsl/gsl>

namespace sp {

auto load_string_file(const std::filesystem::path& path) -> std::string
{
   std::ifstream file{path, std::ios::binary | std::ios::ate};

   if (!file.is_open()) {
      throw compose_exception<std::runtime_error>("Failed to open file: ", path, '.');
   }

   const auto size = file.tellg();

   std::string buf;
   buf.resize(gsl::narrow_cast<std::size_t>(size));

   file.seekg(0);
   file.read(buf.data(), size);

   return buf;
}

}
