#pragma once

#include "smart_win32_handle.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <tuple>

#include <gsl/gsl>

#include <Windows.h>

namespace sp::win32 {

class Memeory_mapped_file {
public:
   enum class Mode { read, read_write };

   Memeory_mapped_file() = default;

   Memeory_mapped_file(const std::filesystem::path& path, const Mode mode = Mode::read)
   {
      if (!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
         throw std::runtime_error{"File does not exist."};
      }

      const auto file_size = std::filesystem::file_size(path);

      if (file_size > std::numeric_limits<std::uint32_t>::max()) {
         throw std::runtime_error{"File too large."};
      }

      _size = static_cast<std::uint32_t>(file_size);

      const auto desired_access =
         mode == Mode::read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;

      const auto file = Unique_handle{
         CreateFileW(path.wstring().c_str(), desired_access, FILE_SHARE_READ,
                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};

      if (file.get() == INVALID_HANDLE_VALUE) {
         throw std::invalid_argument{"Unable to open file."};
      }

      const auto page_access = mode == Mode::read ? PAGE_READONLY : PAGE_READWRITE;

      const auto file_mapping = Unique_handle{
         CreateFileMappingW(file.get(), nullptr, page_access, 0, 0, nullptr)};

      if (file_mapping == nullptr) {
         throw std::runtime_error{"Unable to create file mapping."};
      }

      const auto file_map_desired_access =
         mode == Mode::read ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE;

      _view = {static_cast<std::byte*>(
                  MapViewOfFile(file_mapping.get(), file_map_desired_access, 0, 0, 0)),
               &unmap};

      if (_view == nullptr) {
         throw std::runtime_error{"Unable to create view of file mapping."};
      }
   }

   Memeory_mapped_file(const Memeory_mapped_file&) = delete;
   Memeory_mapped_file& operator=(const Memeory_mapped_file&) = delete;

   Memeory_mapped_file(Memeory_mapped_file&&) = default;
   Memeory_mapped_file& operator=(Memeory_mapped_file&&) = default;

   ~Memeory_mapped_file() = default;

   gsl::span<const std::byte> bytes() const noexcept
   {
      return gsl::make_span(_view.get(), _size);
   }

private:
   static void unmap(std::byte* mapping) noexcept
   {
      if (mapping) UnmapViewOfFile(mapping);
   }

   std::unique_ptr<std::byte, decltype(&unmap)> _view{nullptr, &unmap};
   std::uint32_t _size = 0;
};

}
