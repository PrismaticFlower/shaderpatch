
#include "munge_spfx.hpp"
#include "compose_exception.hpp"
#include "envfx_edits.hpp"
#include "munge_colorgrading_regions.hpp"
#include "req_file_helpers.hpp"
#include "string_utilities.hpp"
#include "synced_io.hpp"
#include "volume_resource.hpp"

#include <filesystem>
#include <fstream>
#include <string_view>

#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <glm/gtc/quaternion.hpp>

namespace sp {

namespace fs = std::filesystem;
using namespace std::literals;

namespace {

auto regions_file_path(const fs::path& spfx_path) -> fs::path
{
   const auto filename = spfx_path.stem() += L"_colorgrading.rgn"sv;
   auto rgn_path = spfx_path;

   rgn_path.replace_filename(filename);

   return rgn_path;
}

bool regions_file_exists(const fs::path& rgn_path) noexcept
{
   try {
      return fs::exists(rgn_path) && fs::is_regular_file(rgn_path);
   }
   catch (std::exception&) {
      return false;
   }
}

auto read_spfx_yaml_raw(const fs::path& spfx_path) -> std::vector<std::byte>
{
   // Test that the file is valid YAML.
   YAML::LoadFile(spfx_path.string());

   std::ifstream file{spfx_path, std::ios::ate};

   std::vector<std::byte> data;
   data.resize(static_cast<std::size_t>(file.tellg()));

   file.seekg(0);
   file.read(reinterpret_cast<char*>(data.data()), data.size());

   return data;
}

void generate_spfx_req_file(YAML::Node spfx, const fs::path& save_path,
                            const fs::path& region_file_path, const bool has_regions)
{
   auto req_path = save_path;
   req_path.replace_extension(".envfx.req"sv);

   std::vector<std::pair<std::string, std::vector<std::string>>> key_sections{
      {"game_envfx"s, {save_path.stem().string()}}};

   if (has_regions) {
      key_sections.emplace_back("color_regions"s,
                                std::vector{region_file_path.stem().string()});
   }

   std::vector<std::string> textures;

   for (auto section : spfx) {
      if (!section.second.IsMap()) continue;

      for (auto key : section.second) {
         if (!key.second.IsScalar()) continue;

         if (make_ci_string(key.first.as<std::string>()).find("texture") !=
             Ci_string::npos) {
            auto texture = key.second.as<std::string>();

            if (texture.empty()) continue;

            textures.emplace_back(std::move(texture));
         }
      }
   }

   key_sections.emplace_back("sptex"s, std::move(textures));

   emit_req_file(req_path, key_sections);
}

}

void munge_spfx(const fs::path& spfx_path, const fs::path& output_dir)
{
   Expects(fs::exists(spfx_path) && fs::is_regular_file(spfx_path));

   auto envfx_path = spfx_path;
   envfx_path.replace_extension(".fx"sv);

   if (!fs::exists(envfx_path) || !fs::is_regular_file(envfx_path)) {
      throw compose_exception<std::runtime_error>("Freestanding .spfx file "sv,
                                                  spfx_path, " found."sv);
   }

   edit_envfx(envfx_path, output_dir);

   synced_print("Munging "sv, spfx_path.filename().string(), "..."sv);

   YAML::Node spfx_node;

   try {
      spfx_node = YAML::LoadFile(spfx_path.string());
   }
   catch (std::exception& e) {
      synced_error_print("Error parsing "sv, spfx_path, " message was: "sv, e.what());

      return;
   }

   const auto data = read_spfx_yaml_raw(spfx_path);

   const auto save_path =
      output_dir / spfx_path.filename().replace_extension(".envfx");

   const auto regions_path = regions_file_path(spfx_path);
   const bool has_regions = regions_file_exists(regions_path);

   if (has_regions) {
      munge_colorgrading_regions(regions_path, regions_path.parent_path(), output_dir);
   }

   save_volume_resource(save_path.string(), spfx_path.stem().string(),
                        Volume_resource_type::fx_config, data);

   generate_spfx_req_file(spfx_node, save_path, regions_path, has_regions);
}
}
