
#include "munge_spfx.hpp"
#include "compose_exception.hpp"
#include "envfx_edits.hpp"
#include "req_file_helpers.hpp"
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

namespace sp {

namespace fs = std::filesystem;
using namespace std::literals;

namespace {

auto read_spfx_yaml(const fs::path& spfx_path) -> std::vector<std::byte>
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

void generate_spfx_req_file(const fs::path& save_path)
{
   auto req_path = save_path;
   req_path.replace_extension(".envfx.req"sv);

   emit_req_file(req_path, {{"game_envfx"s, {save_path.stem().string()}}});
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

   const auto data = read_spfx_yaml(spfx_path);

   const auto save_path =
      output_dir / spfx_path.filename().replace_extension(".envfx");

   save_volume_resource(save_path.string(), spfx_path.stem().string(),
                        Volume_resource_type::fx_config, data);
   generate_spfx_req_file(save_path);
}
}
