
#include "logger.hpp"
#include "memory_mapped_file.hpp"
#include "unlock_memory.hpp"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <Windows.h>

namespace sp {

using namespace std::literals;

namespace {

auto get_process_exe_path() -> std::wstring
{
   std::wstring file_path{256, L' '};

   do {
      file_path.resize(file_path.size() * 2);
      GetModuleFileNameW(nullptr, file_path.data(), file_path.size());
   } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

   return file_path;
}

auto get_process_base_address() -> std::byte*
{
   return reinterpret_cast<std::byte*>(GetModuleHandleW(nullptr));
}

auto get_sha256(gsl::span<const std::byte> data) noexcept
   -> std::optional<std::vector<std::byte>>
{
   BCRYPT_ALG_HANDLE alg_handle;
   auto alg_finally = gsl::finally([&alg_handle] {
      if (alg_handle) BCryptCloseAlgorithmProvider(alg_handle, 0);
   });

   if (BCryptOpenAlgorithmProvider(&alg_handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
      return std::nullopt;

   BCRYPT_ALG_HANDLE hash_handle;
   auto hash_finally = gsl::finally([&hash_handle] {
      if (hash_handle) BCryptDestroyHash(hash_handle);
   });

   if (BCryptCreateHash(alg_handle, &hash_handle, nullptr, 0, nullptr, 0, 0) < 0)
      return std::nullopt;

   DWORD hash_size{};
   DWORD prop_size{};

   if (BCryptGetProperty(alg_handle, BCRYPT_HASH_LENGTH,
                         reinterpret_cast<PUCHAR>(&hash_size), sizeof(DWORD),
                         &prop_size, 0) < 0)
      return std::nullopt;

   if (BCryptHashData(hash_handle,
                      reinterpret_cast<PUCHAR>(const_cast<std::byte*>(data.data())),
                      gsl::narrow<ULONG>(data.size()), 0) < 0)
      return std::nullopt;

   std::vector<std::byte> hash_data;
   hash_data.resize(hash_size);

   if (BCryptFinishHash(hash_handle, reinterpret_cast<PUCHAR>(hash_data.data()),
                        hash_size, 0) < 0)
      return std::nullopt;

   return hash_data;
}

auto stringify_hash(const std::vector<std::byte>& hash) -> std::string
{
   std::string str;
   str.reserve(hash.size() * 2);

   for (auto byte : hash) {
      auto val = std::to_integer<unsigned>(byte);
      if (val == 0) {
         str.resize(str.size() + 2, '0');
      }
      else if (val <= 0x0f) {
         str.resize(str.size() + 2, '0');
         std::to_chars(&str.back(), &str.back() + 1,
                       std::to_integer<unsigned>(byte), 16);
      }
      else {
         str.resize(str.size() + 2);
         std::to_chars(&str.back() - 1, &str.back() + 1,
                       std::to_integer<unsigned>(byte), 16);
      }
   }

   return str;
}

auto get_process_exe_sha256() noexcept -> std::optional<std::string>
{
   auto hash =
      get_sha256(win32::Memeory_mapped_file{get_process_exe_path()}.bytes());

   if (!hash) std::nullopt;

   return stringify_hash(*hash);
}

}

void fps_unlock() noexcept
{
   auto hash_str = get_process_exe_sha256();

   if (!hash_str) {
      log(Log_level::warning,
          "Failed to get SHA256 of process executable, unable to unlock FPS.");

      return;
   }

   auto config = YAML::LoadFile("data/shaderpatch/fps limit info.yml"s);

   if (!config[*hash_str]) {
      log(Log_level::warning, "Unknown battlefrontii.exe, unable to unlock FPS."sv);

      return;
   }

   auto* base_address = get_process_base_address();
   auto edit_offset = config[*hash_str]["offset"s].as<std::int32_t>();
   auto expected_value = config[*hash_str]["expected"s].as<std::int32_t>();
   auto replacement_value = config[*hash_str]["replacement"s].as<std::int32_t>();

   auto* address = base_address + edit_offset;

   auto mem_guard = unlock_memory(address, sizeof(replacement_value));

   if (std::memcmp(address, &expected_value, sizeof(expected_value)) != 0) {
      log(Log_level::warning,
          "Unexpected value at address for FPS unlock, refusing to apply FPS unlock patch."sv);

      return;
   }

   std::memcpy(address, &replacement_value, sizeof(replacement_value));

   log(Log_level::info, "FPS unlock patch applied."sv);
}

}
