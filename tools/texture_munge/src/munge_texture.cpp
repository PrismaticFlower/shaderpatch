
#include "compose_exception.hpp"
#include "patch_texture_io.hpp"
#include "process_image.hpp"
#include "synced_io.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <glm/glm.hpp>
#include <gsl/gsl>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)

#include <yaml-cpp/yaml.h>
#pragma warning(pop)

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;
namespace DX = DirectX;

namespace {

constexpr auto mip_res(std::size_t res, std::size_t level) noexcept
{
   return std::max(res >> level, std::size_t{1u});
}

void check_sampler_info(const YAML::Node& config)
{
   if (config["WrapMode"s]) {
      synced_print("Note! Ignoring texture old parameter \"WrapMode\".");
   }

   if (config["Filter"s]) {
      synced_print("Note! Ignoring texture old parameter \"Filter\".");
   }
}

auto get_texture_info(const DX::ScratchImage& image) -> Texture_info
{
   Expects(image.GetMetadata().arraySize == 1 && image.GetMetadata().depth == 1);

   const auto meta = image.GetMetadata();

   Texture_info info;

   info.type = Texture_type::texture2d;
   info.width = gsl::narrow_cast<std::uint32_t>(meta.width);
   info.height = gsl::narrow_cast<std::uint32_t>(meta.height);
   info.depth = 1;
   info.mip_count = gsl::narrow_cast<std::uint32_t>(meta.mipLevels);
   info.array_size = 1;
   info.format = meta.format;

   return info;
}

auto get_texture_data(const DX::ScratchImage& image) -> std::vector<Texture_data>
{
   std::vector<Texture_data> texture_data;

   const auto& meta = image.GetMetadata();

   texture_data.reserve(meta.mipLevels * meta.arraySize);

   for (auto index = 0; index < meta.arraySize; ++index) {
      for (auto mip = 0; mip < meta.mipLevels; ++mip) {
         auto* const sub_image = image.GetImage(mip, index, 0);
         const auto depth = mip_res(meta.depth, mip);

         Texture_data data;
         data.pitch = gsl::narrow_cast<UINT>(sub_image->rowPitch);
         data.slice_pitch = gsl::narrow_cast<UINT>(sub_image->slicePitch);
         data.data = gsl::make_span(reinterpret_cast<std::byte*>(sub_image->pixels),
                                    sub_image->slicePitch * depth);

         texture_data.emplace_back(data);
      }
   }

   return texture_data;
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

      check_sampler_info(config);

      auto image = process_image(config, image_file_path);

      const auto file_type = config["_SP_DirectTexture"s].as<bool>(false)
                                ? Texture_file_type::direct_texture
                                : Texture_file_type::volume_resource;

      write_patch_texture(output_file_path, get_texture_info(image),
                          get_texture_data(image), file_type);
   }
   catch (std::exception& e) {
      synced_error_print("Error while munging "sv,
                         image_file_path.filename().string(), " :"sv, e.what());
   }
}
}
