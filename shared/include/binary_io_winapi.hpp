#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <type_traits>

#include <Windows.h>

namespace sp {

template<typename T>
concept Binary_io_trivial = (std::is_default_constructible_v<T> &&
                             std::is_trivially_copyable_v<T>);

// clang-format off
template<typename T>
concept Binary_io_trivial_range =
   Binary_io_trivial<std::ranges::range_value_t<T>> && requires(T& range) {
      { range.data() } -> std::same_as<std::add_pointer_t<std::ranges::range_value_t<T>>>;
      { range.size() } -> std::convertible_to<std::size_t>; 
};
// clang-format on

using Binary_file_handle =
   std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype([](HANDLE file) noexcept {
                      if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
                   })>;

class Binary_file : public Binary_file_handle {
public:
   enum class Mode { read, write };

   Binary_file() = default;

   Binary_file(const std::filesystem::path& path, const Mode mode)
   {
      reset(CreateFileW(path.c_str(), mode == Mode::read ? GENERIC_READ : GENERIC_WRITE,
                        0x0, nullptr, mode == Mode::read ? OPEN_EXISTING : CREATE_ALWAYS,
                        FILE_FLAG_SEQUENTIAL_SCAN, nullptr));

      if (get() == INVALID_HANDLE_VALUE) {
         throw std::runtime_error{"failed to create file"};
      }
   }

   operator HANDLE() const noexcept
   {
      return get();
   }
};

class Binary_writer {
public:
   explicit Binary_writer(const std::filesystem::path& path)
      : _file{path, Binary_file::Mode::write}
   {
   }

   template<Binary_io_trivial T>
   void write(const T& v)
   {
      DWORD bytes_written{};

      if (!WriteFile(_file, &v, sizeof(T), &bytes_written, nullptr)) {
         throw std::runtime_error{"failed to write to file"};
      }
   }

   template<Binary_io_trivial_range T>
   void write(const T& range)
   {
      DWORD bytes_written{};

      if (!WriteFile(_file, range.data(),
                     sizeof(std::ranges::range_value_t<T>) * range.size(),
                     &bytes_written, nullptr)) {
         throw std::runtime_error{"failed to write to file"};
      }
   }

   template<typename... T>
   void write(const T&... values)
   {
      (write(values), ...);
   }

   void close() noexcept
   {
      _file.reset(nullptr);
   }

private:
   Binary_file _file;
};

class Binary_reader {
public:
   explicit Binary_reader(const std::filesystem::path& path)
      : _file{path, Binary_file::Mode::read}
   {
   }

   template<Binary_io_trivial T>
   auto read() -> T
   {
      T value{};

      DWORD bytes_read{};

      if (!ReadFile(_file, &value, sizeof(T), &bytes_read, nullptr)) {
         throw std::runtime_error{"failed to read from file"};
      }

      return value;
   }

   template<Binary_io_trivial T>
   void read_to(T& out)
   {
      out = read<T>();
   }

   template<Binary_io_trivial_range T>
   void read_to(T& out)
   {
      DWORD bytes_read{};

      if (!ReadFile(_file, out.data(),
                    out.size() * sizeof(std::ranges::range_value_t<T>),
                    &bytes_read, nullptr)) {
         throw std::runtime_error{"failed to read from file"};
      }
   }

   template<typename... T>
   void read_to(T&... values)
   {
      (read_to(values), ...);
   }

private:
   Binary_file _file;
};

}
