
#include "envfx_edits.hpp"
#include "compose_exception.hpp"
#include "config_file.hpp"
#include "synced_io.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace sp {

namespace fs = std::filesystem;
using namespace std::literals;

namespace {

auto load_fx_cfg(const fs::path& fx_path) -> cfg::Node
{
   std::ifstream file{fx_path};

   try {
      return cfg::from_istream(file);
   }
   catch (Parse_error& e) {
      throw compose_exception<std::runtime_error>(
         "Error occured while parsing ", fx_path, ". Message: ", e.what());
   }
}

auto save_fx_cfg(const cfg::Node& fx_node, const fs::path& path)
{
   std::ofstream file{path};

   file << fx_node;
}

void apply_hdr_edits(cfg::Node& fx_node)
{
   for (auto it = std::cbegin(fx_node); it != std::cend(fx_node); ++it) {

      if (it->first != "Effect"_svci) continue;
      if (it->second.values_count() == 0) continue;
      if (it->second.get_value<std::string>() != "HDR"_svci) continue;

      it = fx_node.erase(it);
   }

   cfg::Node hdr_node;

   hdr_node.emplace_comment("Auto-Generated HDR Config Values for Shader Patch"sv);
   hdr_node.emplace_value("HDR");
   hdr_node.emplace_back("Enable"s, cfg::Node{}).second.emplace_value(1);
   hdr_node.emplace_back("DownSizeFactor"s, cfg::Node{}).second.emplace_value(0.25f);
   hdr_node.emplace_back("NumBloomPasses"s, cfg::Node{}).second.emplace_value(0);
   hdr_node.emplace_back("MaxTotalWeight"s, cfg::Node{}).second.emplace_value(0.0f);
   hdr_node.emplace_back("GlowThreshold"s, cfg::Node{}).second.emplace_value(1.0f);
   hdr_node.emplace_back("GlowFactor"s, cfg::Node{}).second.emplace_value(1.0f);

   fx_node.emplace_back("Effect"s, std::move(hdr_node));
}

}

void edit_envfx(const fs::path& fx_path, const fs::path& output_dir)
{
   synced_print("Editing "sv, fx_path.filename().string(), " for Shader Patch..."sv);

   auto fx_cfg = load_fx_cfg(fx_path);

   apply_hdr_edits(fx_cfg);

   auto output_path =
      output_dir / fx_path.filename().replace_extension(".tmpfx");

   save_fx_cfg(fx_cfg, output_path);
}

}
