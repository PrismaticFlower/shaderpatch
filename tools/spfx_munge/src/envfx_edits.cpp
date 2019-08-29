
#include "envfx_edits.hpp"
#include "compose_exception.hpp"
#include "config_file.hpp"
#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace sp {

namespace fs = std::filesystem;
using namespace std::literals;

namespace {

void apply_hdr_edits(cfg::Node& fx_node)
{
   for (auto it = fx_node.begin(); it != fx_node.end(); ++it) {

      if (it->first != "Effect"_svci) continue;
      if (it->second.values_count() == 0) continue;
      if (it->second.get_value<std::string>() != "HDR"_svci) continue;

      it = fx_node.erase(it);

      if (it == fx_node.end()) break;
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

   auto fx_cfg = cfg::load_file(fx_path);

   apply_hdr_edits(fx_cfg);

   auto output_path =
      output_dir / fx_path.filename().replace_extension(".tmpfx");

   cfg::save_file(fx_cfg, output_path);
}

}
