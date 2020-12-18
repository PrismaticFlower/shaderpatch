#include "framework.hpp"

#include "game_locator.hpp"

using namespace std::literals;

namespace {

auto reg_get_string(HKEY key, const wchar_t* const subkey,
                    const wchar_t* const value) -> std::wstring
{
   std::wstring str;

   DWORD data_size = str.size() * sizeof(wchar_t);
   LSTATUS status = ERROR_SUCCESS;

   while ((status = RegGetValueW(key, subkey, value, RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
                                 nullptr, str.data(), &data_size)) == ERROR_MORE_DATA) {
      str.resize(data_size / 2);
   }

   if (status != ERROR_SUCCESS) {
      return {};
   }

   str.resize(str.size() - 1); // drop the null terminator

   return str;
}

auto gog_install_lookup() -> std::optional<std::filesystem::path>
{
   auto path_data =
      reg_get_string(HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\WOW6432Node\GOG.com\Games\1421404701)", L"path");

   if (path_data.empty()) return std::nullopt;

   auto path = std::filesystem::path{path_data.data()} /
               std::filesystem::path{LR"(GameData\BattlefrontII.exe)"};
   path.make_preferred();

   if (!std::filesystem::exists(path)) return std::nullopt;

   return path;
}

auto steam_install_lookup() -> std::optional<std::filesystem::path>
{
   auto path_data =
      reg_get_string(HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 6060)",
                     L"InstallLocation");

   if (path_data.empty()) return std::nullopt;

   auto path = std::filesystem::path{path_data.data()} /
               std::filesystem::path{LR"(GameData\BattlefrontII.exe)"};
   path.make_preferred();

   if (!std::filesystem::exists(path)) return std::nullopt;

   return path;
}

auto disk_install_lookup() -> std::optional<std::filesystem::path>
{
   auto path_data =
      reg_get_string(HKEY_LOCAL_MACHINE,
                     LR"(HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\LucasArts\Star Wars Battlefront II\1.0)",
                     L"ExePath");

   if (path_data.empty()) return std::nullopt;

   std::filesystem::path path = path_data.data();
   path.make_preferred();

   if (!std::filesystem::exists(path)) return std::nullopt;

   return path;
}

void brute_force_disk_search(std::function<void(std::filesystem::path)> found_callback,
                             std::atomic_bool& cancel_search) noexcept
{
   std::wstring str;

   auto size = str.size();

   while ((size = GetLogicalDriveStringsW(str.size(), str.data())) > str.size()) {
      str.resize(size);
   }

   str.resize(size);

   std::vector<std::filesystem::path> drive_paths;

   while (!str.empty()) {
      auto offset = str.find(L'\0');

      drive_paths.emplace_back(str.substr(0, offset));
      str = str.substr(offset + 1);
   }

   std::for_each(std::execution::par, drive_paths.cbegin(), drive_paths.cend(), [&](const auto& drive) {
      for (auto& entry : std::filesystem::recursive_directory_iterator{
              drive, std::filesystem::directory_options::skip_permission_denied}) {
         if (cancel_search) return;

         try {
            if (!entry.is_regular_file()) continue;

            constexpr auto executable = L"battlefrontii.exe"sv;
            const std::wstring filename = entry.path().filename();

            if (CompareStringOrdinal(filename.data(), filename.size(),
                                     executable.data(), executable.size(),
                                     true) == CSTR_EQUAL) {
               found_callback(entry.path());
            }
         }
         catch (std::filesystem::filesystem_error&) {
         }
      }
   });
}

}

auto search_registry_locations_for_game_installs() noexcept
   -> std::vector<std::filesystem::path>

{
   std::vector<std::filesystem::path> paths;
   paths.reserve(3);

   if (auto path = gog_install_lookup(); path) paths.push_back(*path);
   if (auto path = steam_install_lookup(); path) paths.push_back(*path);
   if (auto path = disk_install_lookup(); path) paths.push_back(*path);

   return paths;
}

game_disk_searcher::game_disk_searcher()
{
   search_future = std::async(std::launch::async, [this] {
      brute_force_disk_search(
         [this](std::filesystem::path path) {
            std::scoped_lock lock{discovered_mutex};
            discovered_buffer.push_back(std::move(path));
         },
         cancel_search);

      completed_search = true;
   });
}

game_disk_searcher::~game_disk_searcher()
{
   cancel_search = true;
   search_future.wait();
}

void game_disk_searcher::get_discovered(std::function<void(std::filesystem::path)> callback) noexcept
{
   std::scoped_lock lock{discovered_mutex};

   for (auto& path : discovered_buffer) callback(std::move(path));

   discovered_buffer.clear();
}

bool game_disk_searcher::completed() const noexcept
{
   return completed_search;
}
