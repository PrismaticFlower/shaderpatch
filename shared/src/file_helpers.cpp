#pragma once

#include "file_helpers.hpp"
#include "compose_exception.hpp"

#include <fstream>

#include <gsl/gsl>

namespace sp {

auto load_string_file(const std::filesystem::path& path) -> std::string
{
   std::ifstream file{path, std::ios::binary};

   if (!file.is_open()) {
      throw compose_exception<std::runtime_error>("Failed to open file: ", path, '.');
   }

   const auto size = std::filesystem::file_size(path);

   std::string buf;
   buf.resize(gsl::narrow_cast<std::size_t>(size));

   file.read(buf.data(), size);

   return buf;
}

}
