
#include "munge_terrain_materials.hpp"
#include "memory_mapped_file.hpp"
#include "synced_io.hpp"
#include "terrain_modelify.hpp"
#include "ucfb_reader.hpp"
#include "utility.hpp"

#include <stdexcept>
#include <vector>

using namespace std::literals;
namespace fs = std::filesystem;

namespace sp {

namespace {
struct Terrain_material {
   std::string albedo_map;
   std::string normal_map;
   std::string metallic_roughness_map;
   std::string ao_map;
   std::string height_map;

   bool use_parallax_occlusion_mapping;
};

struct Terrain_materials_config {
   bool use_envmap = false;
   std::string envmap_name;

   bool use_height_based_blending = true;

   bool use_global_detail_map = false;
   std::string global_detail_map;

   std::unordered_map<std::string, Terrain_material> materials;
};

auto get_terrain_texture_names(const fs::path& path) -> std::vector<std::string>
{
   const auto file_mapping =
      win32::Memeory_mapped_file{path, win32::Memeory_mapped_file::Mode::read};

   ucfb::Reader reader{file_mapping.bytes()};

   auto tern = reader.read_child_strict<"tern"_mn>();

   const auto [grid_unit_size, height_scale, height_floor, height_ceiling,
               grid_size, height_patches, texture_patches, texture_count] =
      tern.read_child_strict<"INFO"_mn>()
         .read_multi_unaligned<float, float, float, float, std::uint16_t,
                               std::uint16_t, std::uint16_t, std::uint16_t>();

   std::vector<std::string> textures;
   textures.reserve(texture_count);

   auto ltex = tern.read_child_strict<"LTEX"_mn>();

   for (auto i = 0; i < texture_count; ++i) {
      textures.emplace_back(ltex.read_string());
   }

   return textures;
}
}

void munge_terrain_materials(const std::unordered_map<Ci_string, std::filesystem::path>& source_files,
                             const std::filesystem::path& output_munge_files_dir,
                             const std::filesystem::path& input_munge_files_dir,
                             const std::filesystem::path& input_sptex_files_dir) noexcept
{
   for (const auto& file : source_files) {
      try {
         if (file.second.extension() != ".tmtrl"_svci) continue;

         const auto terrain_file_name =
            file.second.stem().replace_extension(".terrain"sv);

         const auto input_file_path = input_munge_files_dir / terrain_file_name;
         const auto output_file_path = output_munge_files_dir / terrain_file_name;

         if (const auto last_write_time =
                safe_max(fs::last_write_time(file.second),
                         fs::last_write_time(input_file_path));
             fs::exists(output_file_path) &&
             (last_write_time < fs::last_write_time(output_file_path))) {
            continue;
         }

         terrain_modelify(input_file_path, output_file_path);
      }
      catch (std::exception& e) {
         synced_error_print("Error munging "sv, file.first, ": "sv, e.what());
      }
   }
}
}
