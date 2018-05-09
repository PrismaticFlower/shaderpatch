
#include "compose_exception.hpp"
#include "patch_texture.hpp"
#include "process_image.hpp"
#include "synced_io.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;

namespace {

auto get_sampler_info(YAML::Node config) -> Sampler_info
{
   Sampler_info info;

   info.srgb = config["sRGB"s].as<bool>();

   if (const auto wrap_mode = config["WrapMode"s].as<std::string>();
       wrap_mode == "clamp"sv) {
      info.address_mode_u = D3DTADDRESS_CLAMP;
      info.address_mode_v = D3DTADDRESS_CLAMP;
      info.address_mode_w = D3DTADDRESS_CLAMP;
   }
   else if (wrap_mode == "repeat"sv) {
      info.address_mode_u = D3DTADDRESS_WRAP;
      info.address_mode_v = D3DTADDRESS_WRAP;
      info.address_mode_w = D3DTADDRESS_WRAP;
   }
   else if (wrap_mode == "mirror"sv) {
      info.address_mode_u = D3DTADDRESS_MIRROR;
      info.address_mode_v = D3DTADDRESS_MIRROR;
      info.address_mode_w = D3DTADDRESS_MIRROR;
   }
   else {
      throw compose_exception<std::runtime_error>("Invalid WrapMode in config "sv,
                                                  std::quoted(wrap_mode), '.');
   }

   if (const auto filter = config["Filter"s].as<std::string>(); filter == "point"sv) {
      info.mag_filter = D3DTEXF_POINT;
      info.min_filter = D3DTEXF_POINT;
      info.mip_filter = D3DTEXF_POINT;
   }
   else if (filter == "linear"sv) {
      info.mag_filter = D3DTEXF_LINEAR;
      info.min_filter = D3DTEXF_LINEAR;
      info.mip_filter = D3DTEXF_LINEAR;
   }
   else if (filter == "anioscoptic"sv) {
      info.mag_filter = D3DTEXF_LINEAR;
      info.min_filter = D3DTEXF_ANISOTROPIC;
      info.mip_filter = D3DTEXF_LINEAR;
   }
   else {
      throw compose_exception<std::runtime_error>("Invalid Filter in config "sv,
                                                  std::quoted(filter), '.');
   }

   info.mip_lod_bias = 0.0f;

   return info;
}
}

void munge_texture(fs::path config_file_path, const fs::path& output_dir) noexcept
{
   Expects(fs::is_directory(output_dir) && fs::is_regular_file(config_file_path));

   auto image_file_path = config_file_path;
   image_file_path.replace_extension(""sv);

   if (!fs::exists(image_file_path) || !fs::is_regular_file(image_file_path)) {
      synced_print("Warning freestanding texture config file "sv,
                   config_file_path, '.');

      return;
   }

   const auto output_file_path =
      output_dir / image_file_path.stem().replace_extension(".sptex"s);

   if (fs::exists(output_file_path) &&
       (fs::last_write_time(config_file_path) < fs::last_write_time(output_file_path)) &&
       (fs::last_write_time(image_file_path) < fs::last_write_time(output_file_path))) {
      return;
   }

   try {
      synced_print("Munging (for Shader Patch) "sv,
                   image_file_path.filename().string());

      auto config = YAML::LoadFile(config_file_path.string());

      auto [texture_info, mip_levels] = process_image(config, image_file_path);
      auto sampler_info = get_sampler_info(config);

      write_patch_texture(output_file_path, texture_info, sampler_info, mip_levels);
   }
   catch (std::exception& e) {
      synced_error_print("Error while munging "sv,
                         image_file_path.filename().string(), " :"sv, e.what());
   }
}
}
