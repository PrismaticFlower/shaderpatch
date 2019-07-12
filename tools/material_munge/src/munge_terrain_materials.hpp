
#include "string_utilities.hpp"

#include <filesystem>
#include <unordered_map>

namespace sp {

void munge_terrain_materials(
   const std::unordered_map<Ci_string, std::filesystem::path>& source_files,
   const std::filesystem::path& output_munge_files_dir,
   const std::filesystem::path& input_munge_files_dir,
   const std::filesystem::path& input_sptex_files_dir) noexcept;

}
