#pragma once

#include "process_image.hpp"
#include "compose_exception.hpp"
#include "format_helpers.hpp"

#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <string_view>

#include <Compressonator.h>
#include <DirectXTex.h>
#include <glm/glm.hpp>
#include <gsl/gsl>

namespace sp {

using namespace std::literals;
namespace fs = boost::filesystem;
namespace DX = DirectX;

namespace {
constexpr auto min_resolution = 4u;

constexpr bool is_power_of_2(glm::uint i)
{
   return i && !(i & (i - 1));
}

constexpr glm::uint round_next_power_of_2(glm::uint i)
{
   i--;
   i |= i >> 1;
   i |= i >> 2;
   i |= i >> 4;
   i |= i >> 8;
   i |= i >> 16;
   i++;

   return i;
}

void check_image_resolution(const DX::ScratchImage& image)
{
   if (image.GetMetadata().width > min_resolution) return;
   if (image.GetMetadata().height > min_resolution) return;

   throw compose_exception<std::invalid_argument>("Image too small is {"sv,
                                                  image.GetMetadata().width, ',',
                                                  image.GetMetadata().height,
                                                  "} minimum size is {"sv, min_resolution,
                                                  ',', min_resolution, "}."sv);
}

auto calculate_mip_count(glm::uvec2 resolution)
{
   Expects(is_power_of_2(resolution.x) && is_power_of_2(resolution.y));

   std::size_t mip_count = 1;

   while (std::min({resolution.x, resolution.y}) != min_resolution) {
      ++mip_count;

      resolution /= 2u;
   }

   return mip_count;
}

auto get_mip_filter_type(YAML::Node config) -> DX::TEX_FILTER_FLAGS
{
   int flags = DX::TEX_FILTER_CUBIC | DX::TEX_FILTER_FORCE_NON_WIC;

   const auto wrap_mode = config["WrapMode"s].as<std::string>();

   if (wrap_mode == "repeat"sv) {
      flags |= DX::TEX_FILTER_WRAP;
   }
   else if (wrap_mode == "mirror"sv) {
      flags |= DX::TEX_FILTER_MIRROR;
   }
   else if (wrap_mode != "clamp"sv) {
      throw compose_exception<std::runtime_error>("Invalid WrapMode in config "sv,
                                                  std::quoted(wrap_mode), '.');
   }

   return static_cast<DX::TEX_FILTER_FLAGS>(flags);
}

auto load_image(const fs::path& image_file_path, bool srgb) -> DirectX::ScratchImage
{
   DirectX::ScratchImage loaded_image;

   if (FAILED(DirectX::LoadFromTGAFile(image_file_path.c_str(), nullptr, loaded_image))) {
      throw std::runtime_error{"Unable to open texture file."s};
   }

   if (srgb) {
      loaded_image.OverrideFormat(DX::MakeSRGB(loaded_image.GetMetadata().format));
   }

   return loaded_image;
}

auto swizzle_pixel_format(DX::ScratchImage image) -> DX::ScratchImage
{
   DXGI_FORMAT convert_to_format = image.GetMetadata().format;

   switch (image.GetMetadata().format) {
   case DXGI_FORMAT_R8G8B8A8_UNORM:
      convert_to_format = DXGI_FORMAT_B8G8R8A8_UNORM;
      break;
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      convert_to_format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
      break;
   default:
      return image;
   }

   DX::ScratchImage converted_image;

   if (DX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                   convert_to_format, DX::TEX_FILTER_DEFAULT | DX::TEX_FILTER_FORCE_NON_WIC,
                   0.0f, converted_image)) {
      throw std::runtime_error{
         "Failed to swizzle pixel format from RGBA to BGRA for Direct3D 9."s};
   }

   return converted_image;
}

auto make_image_power_of_2(DX::ScratchImage image) -> DX::ScratchImage
{
   glm::uvec2 image_res{image.GetMetadata().width, image.GetMetadata().height};

   if (!is_power_of_2(image_res.x) || !is_power_of_2(image_res.y)) {
      DX::ScratchImage resized_image;

      if (FAILED(DX::Resize(image.GetImages(), image.GetImageCount(),
                            image.GetMetadata(), round_next_power_of_2(image_res.x),
                            round_next_power_of_2(image_res.y),
                            DX::TEX_FILTER_CUBIC | DX::TEX_FILTER_FORCE_NON_WIC,
                            resized_image))) {
         throw std::runtime_error{"Failed to resize non-power of 2 texture."s};
      }

      return resized_image;
   }

   return image;
}

auto mipmap_image(DX::ScratchImage image, DX::TEX_FILTER_FLAGS filter) -> DX::ScratchImage
{
   DX::ScratchImage mipped_image;

   const glm::uvec2 image_res{image.GetMetadata().width, image.GetMetadata().height};

   if (FAILED(DX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                  image.GetMetadata(), filter,
                                  calculate_mip_count(image_res), mipped_image))) {
      throw std::runtime_error{"Unable to generate mip maps texture file."s};
   }

   return mipped_image;
}

auto get_texture_info(const DX::ScratchImage& image) -> Texture_info
{
   const auto meta = image.GetMetadata();

   Texture_info info;

   info.width = meta.width;
   info.height = meta.height;
   info.mip_count = meta.mipLevels;
   info.format = dxgi_format_to_d3dformat(meta.format);

   return info;
}

auto compress_image(const DX::ScratchImage& image, YAML::Node config)
   -> std::tuple<std::vector<std::vector<std::byte>>, D3DFORMAT>
{
   CMP_CompressOptions compress_options{sizeof(CMP_CompressOptions)};

   compress_options.bUseChannelWeighting = true;
   compress_options.fWeightingRed = 0.3086;
   compress_options.fWeightingGreen = 0.6094;
   compress_options.fWeightingBlue = 0.0820;
   compress_options.bUseAdaptiveWeighting = true;
   compress_options.bDXT1UseAlpha = true;
   compress_options.bUseGPUDecompress = false;
   compress_options.bUseGPUCompress = false;
   compress_options.nAlphaThreshold = 0x7f;
   compress_options.nCompressionSpeed = CMP_Speed_Normal;
   compress_options.fquality = 1.0;

   const auto [target_format, d3d_target_format] =
      read_config_format(config["CompressionFormat"s].as<std::string>("BC3"s));
   const auto source_format = dxgi_format_to_cmp_format(image.GetMetadata().format);

   std::vector<std::vector<std::byte>> mips;

   mips.reserve(image.GetImageCount());

   for (const DX::Image& sub_image :
        gsl::make_span(image.GetImages(), image.GetImageCount())) {
      CMP_Texture source_texture{sizeof(CMP_Texture)};
      source_texture.dwWidth = sub_image.width;
      source_texture.dwHeight = sub_image.height;
      source_texture.dwPitch = sub_image.rowPitch;
      source_texture.format = source_format;
      source_texture.nBlockHeight = 4;
      source_texture.nBlockWidth = 4;
      source_texture.nBlockDepth = 1;
      source_texture.dwDataSize = sub_image.slicePitch;
      source_texture.pData = sub_image.pixels;

      CMP_Texture dest_texture{sizeof(CMP_Texture)};
      dest_texture.dwWidth = sub_image.width;
      dest_texture.dwHeight = sub_image.height;
      dest_texture.dwPitch = 0;
      dest_texture.format = target_format;
      dest_texture.nBlockHeight = 4;
      dest_texture.nBlockWidth = 4;
      dest_texture.nBlockDepth = 1;

      std::vector<std::byte> level;
      level.resize(CMP_CalculateBufferSize(&dest_texture));

      dest_texture.dwDataSize = level.size();
      dest_texture.pData = reinterpret_cast<std::uint8_t*>(level.data());

      CMP_ConvertTexture(&source_texture, &dest_texture, &compress_options,
                         nullptr, 0, 0);

      std::memcpy(level.data(), dest_texture.pData, dest_texture.dwDataSize);

      mips.emplace_back(std::move(level));
   }

   return {mips, d3d_target_format};
}

auto vectorify_image(const DX::ScratchImage& image)
   -> std::vector<std::vector<std::byte>>
{
   std::vector<std::vector<std::byte>> mips;

   mips.reserve(image.GetImageCount());

   for (const DX::Image& sub_image :
        gsl::make_span(image.GetImages(), image.GetImageCount())) {
      std::vector<std::byte> level;
      level.resize(sub_image.slicePitch);

      std::memcpy(level.data(), sub_image.pixels, sub_image.slicePitch);

      mips.emplace_back(std::move(level));
   }

   return mips;
}
}

auto process_image(YAML::Node config, fs::path image_file_path)
   -> std::tuple<Texture_info, std::vector<std::vector<std::byte>>>
{
   Expects(fs::exists(image_file_path) && fs::is_regular_file(image_file_path));

   auto image = load_image(image_file_path, config["sRGB"s].as<bool>(true));

   check_image_resolution(image);

   image = swizzle_pixel_format(std::move(image));
   image = make_image_power_of_2(std::move(image));
   image = mipmap_image(std::move(image), get_mip_filter_type(config));

   auto texture_info = get_texture_info(image);
   std::vector<std::vector<std::byte>> mip_levels;

   if (!config["Uncompressed"s].as<bool>(false)) {
      std::tie(mip_levels, texture_info.format) = compress_image(image, config);
   }
   else {
      mip_levels = vectorify_image(image);
   }

   return {texture_info, mip_levels};
}
}
