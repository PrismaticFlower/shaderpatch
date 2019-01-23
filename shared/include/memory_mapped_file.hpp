#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>

#include <gsl/gsl>

namespace sp::win32 {

class Memeory_mapped_file {
public:
   enum class Mode { read, read_write };

   Memeory_mapped_file() = default;

   Memeory_mapped_file(const std::filesystem::path& path, const Mode mode = Mode::read);

   Memeory_mapped_file(const Memeory_mapped_file&) = delete;
   Memeory_mapped_file& operator=(const Memeory_mapped_file&) = delete;

   Memeory_mapped_file(Memeory_mapped_file&&) = default;
   Memeory_mapped_file& operator=(Memeory_mapped_file&&) = default;

   ~Memeory_mapped_file() = default;

   auto bytes() noexcept -> gsl::span<std::byte>;

   auto bytes() const noexcept -> gsl::span<const std::byte>;

private:
   static void unmap(std::byte* mapping) noexcept;

   std::unique_ptr<std::byte, decltype(&unmap)> _view{nullptr, &unmap};
   std::uint32_t _size = 0;
};

}
