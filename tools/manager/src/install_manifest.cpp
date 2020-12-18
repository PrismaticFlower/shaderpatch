#include "framework.hpp"

using namespace std::literals;

namespace {

constexpr auto manifest_directory = LR"(data\shaderpatch\)"sv;
constexpr auto manifest_file = L"install.manifest"sv;

}

auto load_install_manifest(const std::filesystem::path& base_path) noexcept
   -> std::vector<std::filesystem::path>
{
   std::ifstream file{base_path / manifest_directory / manifest_file};
   std::vector<std::filesystem::path> paths;

   while (file) {
      std::string path;
      std::getline(file, path);

      if (!path.empty()) paths.emplace_back(path);
   }

   return paths;
}

void save_install_manifest(const std::filesystem::path& base_path,
                           const std::vector<std::filesystem::path>& paths) noexcept
{
   std::ofstream file{base_path / manifest_directory / manifest_file};

   for (const auto& path : paths) file << path.string() << '\n';

   file << (std::filesystem::path{manifest_directory} /
            std::filesystem::path{manifest_file})
              .string()
        << '\n';
}
