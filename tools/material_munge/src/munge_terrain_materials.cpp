
#include "munge_terrain_materials.hpp"
#include "compose_exception.hpp"
#include "memory_mapped_file.hpp"
#include "synced_io.hpp"
#include "terrain_assemble_textures.hpp"
#include "terrain_materials_config.hpp"
#include "terrain_modelify.hpp"
#include "terrain_save_material.hpp"
#include "terrain_save_normal_map.hpp"
#include "ucfb_reader.hpp"
#include "utility.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

using namespace std::literals;
namespace fs = std::filesystem;

namespace sp {

namespace {

auto load_terrain(const fs::path& matl_path) -> Terrain_map
{
   fs::path ter_path = matl_path;
   ter_path.replace_extension(".ter"sv);

   try {
      return load_terrain_map(ter_path);
   }
   catch (std::exception&) {
      throw compose_exception<std::runtime_error>("Failed to load terrain file "sv,
                                                  std::quoted(ter_path.string()),
                                                  "!"sv);
   }
}

auto load_terrain_materials_config(const fs::path& path) -> Terrain_materials_config
{
   return YAML::LoadFile(path.string()).as<Terrain_materials_config>();
}

auto get_terrain_textures(const Terrain_map& terrain) -> std::vector<std::string>
{
   std::vector<std::string> result;
   result.reserve(16);

   for (const auto& tex : terrain.texture_names) {
      const auto offset = tex.find_last_of('.');

      if (offset == tex.npos) {
         result.emplace_back(tex);
      }
      else {
         result.emplace_back(tex.substr(0, offset));
      }
   }

   return result;
}

bool should_modelify_terrain(const fs::file_time_type config_last_write_time,
                             const fs::path& input_file_path,
                             const fs::path& output_file_path) noexcept
{
   const auto last_write_time =
      safe_max(config_last_write_time, fs::last_write_time(input_file_path));

   return !(fs::exists(output_file_path) &&
            (last_write_time < fs::last_write_time(output_file_path)));
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

         const auto config_last_write_time = fs::last_write_time(file.second);

         const auto munged_terrain_file_name =
            file.second.stem().replace_extension(".terrain"sv);
         const auto munged_terrain_input_file_path =
            input_munge_files_dir / munged_terrain_file_name;

         const auto terrain_map = load_terrain(file.second);
         const auto terrain_suffix = file.second.stem().string();
         const auto config = load_terrain_materials_config(file.second);

         if (const auto terrain_output_file_path =
                output_munge_files_dir / munged_terrain_file_name;
             should_modelify_terrain(config_last_write_time, munged_terrain_input_file_path,
                                     terrain_output_file_path)) {
            terrain_modelify(terrain_map, terrain_suffix,
                             config.far_terrain == Terrain_far::fullres,
                             config.use_ze_static_lighting,
                             munged_terrain_input_file_path, terrain_output_file_path);
            terrain_save_normal_map(terrain_map, terrain_suffix, output_munge_files_dir);
         }

         const auto terrain_textures = get_terrain_textures(terrain_map);

         terrain_assemble_textures(config, terrain_suffix, terrain_textures,
                                   output_munge_files_dir, input_sptex_files_dir);

         terrain_save_material(config, terrain_map.texture_transforms, terrain_textures,
                               terrain_suffix, output_munge_files_dir);
      }
      catch (std::exception& e) {
         synced_error_print("Error munging "sv, file.first, ": "sv, e.what());
      }
   }
}
}
