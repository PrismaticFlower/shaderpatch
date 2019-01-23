#pragma once

#include "process_image.hpp"
#include "compose_exception.hpp"
#include "format_helpers.hpp"
#include "string_utilities.hpp"

#include <cstring>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string_view>

#include <Compressonator.h>
#include <DirectXTex.h>
#include <comdef.h>
#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>
#include <gsl/gsl>

#include "DirectXTexEXR.h"

namespace sp {

using namespace std::literals;
namespace fs = std::filesystem;
namespace DX = DirectX;

namespace {
constexpr auto min_resolution = 4u;

constexpr bool is_power_of_2(glm::uint i)
{
   return i && !(i & (i - 1));
}

constexpr bool is_multiple_of_4(glm::uint i)
{
   return i && ((i % 4) == 0);
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

constexpr glm::uint round_next_multiple_of_4(glm::uint i)
{
   const auto remainder = i % 4;

   if (remainder != 0) return i + (4 - remainder);

   return i;
}

auto get_mip_count(const glm::uvec2 resolution) noexcept
{
   const auto counts = glm::log2(resolution);

   return glm::max(counts.x, counts.y);
}

auto load_image(const fs::path& image_file_path, bool srgb) -> DirectX::ScratchImage
{
   DirectX::ScratchImage image;
   HRESULT result;

   if (const auto ext = image_file_path.extension().string(); ext == ".dds"_svci) {
      result = DX::LoadFromDDSFile(image_file_path.c_str(), DX::DDS_FLAGS_NONE,
                                   nullptr, image);
   }
   else if (ext == ".hdr"_svci) {
      result = DX::LoadFromHDRFile(image_file_path.c_str(), nullptr, image);
   }
   else if (ext == ".tga"_svci) {
      result = DX::LoadFromTGAFile(image_file_path.c_str(), nullptr, image);
   }
   else if (ext == ".exr"_svci) {
      result = DX::LoadFromEXRFile(image_file_path.c_str(), nullptr, image);
   }
   else {
      result = DX::LoadFromWICFile(image_file_path.c_str(), DX::WIC_FLAGS_NONE,
                                   nullptr, image);
   }

   if (FAILED(result)) {
      throw compose_exception<std::runtime_error>("failed to load texture file reason: "sv,
                                                  _com_error{result}.ErrorMessage());
   }

   if (srgb) image.OverrideFormat(DX::MakeSRGB(image.GetMetadata().format));

   return image;
}

auto remap_roughness_channels(DX::ScratchImage image) -> DirectX::ScratchImage
{
   DX::ScratchImage swapped_image;

   const auto result = TransformImage(
      image.GetImages(), image.GetImageCount(), image.GetMetadata(),
      [](DX::XMVECTOR* out, const DX::XMVECTOR* in, std::size_t width, std::size_t) {
         for (size_t i = 0; i < width; ++i) {
            auto remapped = DX::XMVectorSetByIndex(in[i], 0.0f, DX::XM_SWIZZLE_W);

            out[i] = DX::XMVectorSwizzle<DX::XM_SWIZZLE_W, DX::XM_SWIZZLE_X,
                                         DX::XM_SWIZZLE_W, DX::XM_SWIZZLE_W>(remapped);
         }
      },
      swapped_image);

   if (FAILED(result)) {
      throw std::runtime_error{"Failed to remap colour channels for input roughness map!"s};
   }

   return swapped_image;
}

auto premultiply_alpha(DX::ScratchImage image) -> DX::ScratchImage
{
   DX::ScratchImage converted_image;

   if (DX::PremultiplyAlpha(image.GetImages(), image.GetImageCount(),
                            image.GetMetadata(), DX::TEX_FILTER_DEFAULT,
                            converted_image)) {
      throw std::runtime_error{"Failed to premultiply alpha."s};
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
                            DX::TEX_FILTER_CUBIC, resized_image))) {
         throw std::runtime_error{"Failed to resize non-power of 2 texture."s};
      }

      return resized_image;
   }

   return image;
}

auto make_image_mutliple_of_4(DX::ScratchImage image) -> DX::ScratchImage
{
   glm::uvec2 image_res{image.GetMetadata().width, image.GetMetadata().height};

   if (!is_multiple_of_4(image_res.x) || !is_multiple_of_4(image_res.y)) {
      DX::ScratchImage resized_image;

      if (FAILED(DX::Resize(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                            round_next_multiple_of_4(image_res.x),
                            round_next_multiple_of_4(image_res.y),
                            DX::TEX_FILTER_CUBIC, resized_image))) {
         throw std::runtime_error{
            "Failed to resize non-mutliple of 4 texture for compression"s};
      }

      return resized_image;
   }

   return image;
}

auto mipmap_image(DX::ScratchImage image) -> DX::ScratchImage
{
   DX::ScratchImage mipped_image;

   const glm::uvec2 image_res{image.GetMetadata().width, image.GetMetadata().height};

   if (FAILED(DX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                  image.GetMetadata(), DX::TEX_FILTER_CUBIC,
                                  get_mip_count(image_res), mipped_image))) {
      throw std::runtime_error{"Unable to generate mip maps texture file."s};
   }

   return mipped_image;
}

auto compress_image(const DX::ScratchImage& image, const YAML::Node& config)
   -> DX::ScratchImage
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
   compress_options.dwnumThreads = std::max(std::thread::hardware_concurrency(), 128u);
   compress_options.fquality = 0.8;

   const auto [target_format, dxgi_target_format] =
      read_config_format(config["CompressionFormat"s].as<std::string>("BC7"s));
   const auto source_format = dxgi_format_to_cmp_format(image.GetMetadata().format);

   auto dest_metadata = image.GetMetadata();
   dest_metadata.format = dxgi_target_format;
   DirectX::ScratchImage dest_image;
   dest_image.Initialize(dest_metadata);

   for (auto i = 0; i < dest_image.GetImageCount(); ++i) {
      const auto& src_sub_image = image.GetImages()[i];
      auto& dest_sub_image = dest_image.GetImages()[i];

      CMP_Texture source_texture{sizeof(CMP_Texture)};
      source_texture.dwWidth = src_sub_image.width;
      source_texture.dwHeight = src_sub_image.height;
      source_texture.dwPitch = src_sub_image.rowPitch;
      source_texture.format = source_format;
      source_texture.nBlockHeight = 4;
      source_texture.nBlockWidth = 4;
      source_texture.nBlockDepth = 1;
      source_texture.dwDataSize = src_sub_image.slicePitch;
      source_texture.pData = src_sub_image.pixels;

      CMP_Texture dest_texture{sizeof(CMP_Texture)};
      dest_texture.dwWidth = dest_sub_image.width;
      dest_texture.dwHeight = dest_sub_image.height;
      dest_texture.dwPitch = dest_sub_image.rowPitch;
      dest_texture.format = target_format;
      dest_texture.nBlockHeight = 4;
      dest_texture.nBlockWidth = 4;
      dest_texture.nBlockDepth = 1;
      dest_texture.dwDataSize = dest_sub_image.slicePitch;
      dest_texture.pData = dest_sub_image.pixels;

      CMP_ConvertTexture(&source_texture, &dest_texture, &compress_options,
                         nullptr, 0, 0);
   }

   return dest_image;
}

}

auto process_image(const YAML::Node& config, std::filesystem::path image_file_path)
   -> DX::ScratchImage
{
   Expects(fs::exists(image_file_path) && fs::is_regular_file(image_file_path));

   auto image = load_image(image_file_path, config["sRGB"s].as<bool>(true));

   if (config["Type"s].as<std::string>() == "roughness"sv) {
      image = remap_roughness_channels(std::move(image));
   }

   if (config["PremultiplyAlpha"s].as<bool>(false)) {
      image = premultiply_alpha(std::move(image));
   }

   if (!config["Uncompressed"s].as<bool>(false)) {
      if (config["NoMips"s].as<bool>(false)) {
         image = make_image_mutliple_of_4(std::move(image));
      }
      else {
         image = make_image_power_of_2(std::move(image));
      }
   }

   if (!config["NoMips"s].as<bool>(false)) {
      image = mipmap_image(std::move(image));
   }

   if (!config["Uncompressed"s].as<bool>(false)) {
      image = compress_image(image, config);
   }

   return image;
}
}
