#pragma once

#include "process_image.hpp"
#include "compose_exception.hpp"
#include "format_helpers.hpp"
#include "image_span.hpp"
#include "ispc_texcomp/ispc_texcomp.h"
#include "string_utilities.hpp"
#include "texture_type.hpp"
#include "utility.hpp"

#include <cstring>
#include <execution>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

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

constexpr bool is_power_of_2(std::size_t i)
{
   return i && !(i & (i - 1));
}

constexpr bool is_multiple_of_4(std::size_t i)
{
   return i && ((i % 4u) == 0u);
}

auto get_mip_count(const glm::uvec3 resolution) noexcept
{
   const auto counts = glm::log2(resolution) + 1u;

   return std::max({counts.x, counts.y, counts.z});
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
      throw compose_exception<std::runtime_error>("failed to load image file reason: "sv,
                                                  _com_error{result}.ErrorMessage());
   }

   if (srgb) image.OverrideFormat(DX::MakeSRGB(image.GetMetadata().format));

   return image;
}

auto load_paired_image(fs::path relative_path, const YAML::Node& config)
   -> std::optional<DirectX::ScratchImage>
{
   if (!config["PairedImage"s]) return std::nullopt;

   const auto file_name = config["PairedImage"s].as<std::string>();

   return load_image(relative_path / file_name, false);
}

auto change_image_format(DX::ScratchImage image, const DXGI_FORMAT new_format)
   -> DX::ScratchImage
{
   DX::ScratchImage converted_image;

   if (const auto result =
          DX::Convert(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
                      new_format, DX::TEX_FILTER_FORCE_NON_WIC | DX::TEX_FILTER_DEFAULT,
                      DX::TEX_THRESHOLD_DEFAULT, converted_image);
       FAILED(result)) {
   }

   return converted_image;
}

auto fold_cubemap(DX::ScratchImage image)
{
   Expects(image.GetMetadata().dimension == DX::TEX_DIMENSION_TEXTURE2D);

   auto& meta = image.GetMetadata();

   const auto width = (meta.width / 4);
   const auto height = (meta.height / 3);

   if (width * 4 != meta.width || height * 3 != meta.height) {
      throw compose_exception<std::runtime_error>(
         "Invalid texture dimensions for cubemap!");
   }

   DX::ScratchImage cubemap;
   cubemap.InitializeCube(meta.format, width, height, 1, 1);

   // +X
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {width * 2, height, width, height},
                     *cubemap.GetImage(0, 0, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // -X
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {0, height, width, height},
                     *cubemap.GetImage(0, 1, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Y
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {width, 0, width, height},
                     *cubemap.GetImage(0, 2, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // -Y
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {width, height * 2, width, height},
                     *cubemap.GetImage(0, 3, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Z
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {width, height, width, height},
                     *cubemap.GetImage(0, 4, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   // +Z
   DX::CopyRectangle(*image.GetImage(0, 0, 0), {width * 3, height, width, height},
                     *cubemap.GetImage(0, 5, 0), DX::TEX_FILTER_FORCE_NON_WIC, 0, 0);

   return cubemap;
}

auto remap_roughness_channels(DX::ScratchImage image) -> DirectX::ScratchImage
{
   const auto process_image = [&](Image_span dest_image) noexcept {
      for_each(std::execution::par_unseq, dest_image, [&](const glm::ivec2 index) noexcept {
         auto value = dest_image.load(index);

         value = {0.0f, value.r, 0.0f, 0.0f};

         dest_image.store(index, value);
      });
   };

   for (auto index = 0; index < image.GetMetadata().arraySize; ++index) {
      for (auto mip = 0; mip < image.GetMetadata().mipLevels; ++mip) {
         for (auto slice = 0; slice < image.GetMetadata().depth; ++slice) {
            process_image(*image.GetImage(mip, index, slice));
         }
      }
   }

   return image;
}

auto specular_anti_alias(DX::ScratchImage source_image, DX::ScratchImage normal_map)
   -> DX::ScratchImage
{
   Expects(source_image.GetMetadata().dimension != DX::TEX_DIMENSION_TEXTURE3D);
   Expects(normal_map.GetMetadata().dimension == DX::TEX_DIMENSION_TEXTURE2D);

   const glm::uvec2 normal_map_size{normal_map.GetMetadata().width,
                                    normal_map.GetMetadata().height};

   const auto sample_average_normal =
      [normal_map_span =
          Image_span{*normal_map.GetImage(0, 0, 0)}](const glm::ivec2 position,
                                                     const glm::ivec2 radius) noexcept {
         glm::vec3 normal{};

         const auto offset = position * radius;

         for (auto y = 0; y < radius.y; ++y) {
            for (auto x = 0; x < radius.x; ++x) {
               normal += glm::normalize(
                  normal_map_span.load({offset + glm::ivec2{x, y}}).xyz * 2.0f - 1.0f);
            }
         }

         return normal / static_cast<float>(radius.x * radius.y);
      };

   const auto process_mip = [&](const std::size_t index, const std::size_t mip) noexcept {
      auto dest_image = Image_span{*source_image.GetImage(mip, index, 0)};

      const auto radius = normal_map_size / glm::uvec2{dest_image.size()};

      if (!glm::any(glm::greaterThan(radius, glm::uvec2{1}))) return;

      for_each(std::execution::par_unseq, dest_image, [&](const glm::ivec2 index) noexcept {
         glm::vec3 average_normal = sample_average_normal(index, radius);

         const float r = length(average_normal);
         float k = 10000.0f;

         if (r < 1.f) k = (3.f * r - r * r * r) / (1.f - r * r);

         auto value = dest_image.load(index);

         value.g = glm::sqrt(value.g * value.g + (1.f / k));

         dest_image.store(index, value);
      });
   };

   for (auto index = 0; index < source_image.GetMetadata().arraySize; ++index) {
      for (auto mip = 0; mip < source_image.GetMetadata().mipLevels; ++mip) {
         process_mip(index, mip);
      }
   }

   return source_image;
}

auto premultiply_alpha(DX::ScratchImage image) -> DX::ScratchImage
{
   const auto process_image = [&](Image_span dest_image) noexcept {
      for_each(std::execution::par_unseq, dest_image, [&](const glm::ivec2 index) noexcept {
         auto value = dest_image.load(index);

         value.rgb = value.rgb * value.a;

         dest_image.store(index, value);
      });
   };

   for (auto index = 0; index < image.GetMetadata().arraySize; ++index) {
      for (auto mip = 0; mip < image.GetMetadata().mipLevels; ++mip) {
         for (auto slice = 0; slice < image.GetMetadata().depth; ++slice) {
            process_image(*image.GetImage(mip, index, slice));
         }
      }
   }

   return image;
}

auto make_image_power_of_2(DX::ScratchImage image) -> DX::ScratchImage
{
   glm::uvec2 image_res{image.GetMetadata().width, image.GetMetadata().height};

   if (!is_power_of_2(image_res.x) || !is_power_of_2(image_res.y)) {
      DX::ScratchImage resized_image;

      if (FAILED(DX::Resize(image.GetImages(), image.GetImageCount(),
                            image.GetMetadata(), next_power_of_2(image_res.x),
                            next_power_of_2(image_res.y), DX::TEX_FILTER_CUBIC,
                            resized_image))) {
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

      if (FAILED(DX::Resize(image.GetImages(), image.GetImageCount(),
                            image.GetMetadata(), next_multiple_of<4>(image_res.x),
                            next_multiple_of<4>(image_res.y),
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

   if (image.GetMetadata().dimension == DX::TEX_DIMENSION_TEXTURE3D) {
      if (FAILED(DX::GenerateMipMaps3D(image.GetImages(), image.GetImageCount(),
                                       image.GetMetadata(),
                                       DX::TEX_FILTER_BOX | DX::TEX_FILTER_FORCE_NON_WIC,
                                       0, mipped_image))) {
         throw std::runtime_error{"Unable to generate mip maps"s};
      }
   }
   else if (FAILED(DX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                       image.GetMetadata(),
                                       DX::TEX_FILTER_BOX | DX::TEX_FILTER_FORCE_NON_WIC,
                                       0, mipped_image))) {
      throw std::runtime_error{"Unable to generate mip maps"s};
   }

   return mipped_image;
}

auto mipmap_normalmap(DX::ScratchImage image) noexcept
{
   Expects(image.GetMetadata().dimension != DX::TEX_DIMENSION_TEXTURE3D);

   auto metadata = image.GetMetadata();
   metadata.mipLevels = get_mip_count({metadata.width, metadata.height, 1});
   DX::ScratchImage mipped_image;
   mipped_image.Initialize(metadata);

   const auto copy_and_normalize_image = [&](const UINT index, const UINT mip) noexcept {
      const auto src_image = Image_span{*image.GetImage(mip, index, 0)};
      auto dest_image = Image_span{*mipped_image.GetImage(mip, index, 0)};

      for_each(std::execution::par_unseq, src_image, [&](const glm::ivec2 index) {
         const auto value = src_image.load(index);
         const auto normal = glm::normalize(value.xyz * 2.0f - 1.0f);

         dest_image.store(index, {normal * 0.5f + 0.5f, value.w});
      });
   };

   for (auto index = 0; index < image.GetMetadata().arraySize; ++index) {
      // Copy top level mip.
      copy_and_normalize_image(index, 0);

      for (auto mip = 1; mip < mipped_image.GetMetadata().mipLevels; ++mip) {
         const auto sample_average_normal =
            [upper_image = Image_span{*mipped_image.GetImage(mip - 1, index, 0)}](
               const glm::ivec2 offset) noexcept -> glm::vec4 {
            glm::vec3 normal{};
            float alpha = 0.0f;

            for (auto y = 0; y < 2; ++y) {
               for (auto x = 0; x < 2; ++x) {
                  const auto index = offset + glm::ivec2{x, y};
                  const auto value = upper_image.load(index);

                  normal += glm::normalize(value.xyz * 2.0f - 1.0f);
                  alpha = alpha + (value.w / 4.0f);
               }
            }

            return {glm::normalize(normal) * 0.5f + 0.5f, alpha};
         };

         auto dest_image = Image_span{*mipped_image.GetImage(mip, index, 0)};

         for_each(std::execution::par_unseq, dest_image, [&](const glm::ivec2 index) {
            dest_image.store(index, sample_average_normal(index * 2));
         });
      }
   }

   return mipped_image;
}

auto compress_image(DX::ScratchImage image, const YAML::Node& config) -> DX::ScratchImage
{
   Expects(is_multiple_of_4(image.GetMetadata().width) &&
           is_multiple_of_4(image.GetMetadata().height) &&
           (is_multiple_of_4(image.GetMetadata().depth) ||
            image.GetMetadata().depth == 1));

   const auto src_format = image.GetMetadata().format;

   if (DX::IsCompressed(src_format) || DX::IsPlanar(src_format)) return image;

   const auto [target_format, target_format_dxgi] =
      read_config_format(config["CompressionFormat"s].as<std::string>("BC7"s));

   // Make sure the image format is what the ISPC kernel expects.
   // It'd be nicer to have a better solution for this.
   if (target_format == Compression_format::BC6H_UF16 &&
       src_format != DXGI_FORMAT_R16G16B16A16_FLOAT) {
      image = change_image_format(std::move(image), DXGI_FORMAT_R16G16B16A16_FLOAT);
   }
   else if (make_non_srgb(src_format) != DXGI_FORMAT_R8G8B8A8_UNORM) {
      image = change_image_format(std::move(image), is_srgb(src_format)
                                                       ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                                                       : DXGI_FORMAT_R8G8B8A8_UNORM);
   }

   auto dest_metadata = image.GetMetadata();
   dest_metadata.format = target_format_dxgi;
   DX::ScratchImage dest_image;
   dest_image.Initialize(dest_metadata);

   std::function<void(rgba_surface surface, std::uint8_t* const dest)> compressor;

   if (target_format == Compression_format::BC3) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         CompressBlocksBC3(&surface, dest);
      };
   }
   else if (target_format == Compression_format::BC4) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         CompressBlocksBC4(&surface, dest);
      };
   }
   else if (target_format == Compression_format::BC5) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         CompressBlocksBC5(&surface, dest);
      };
   }
   else if (target_format == Compression_format::BC6H_UF16) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         bc6h_enc_settings settings;
         GetProfile_bc6h_slow(&settings);

         CompressBlocksBC6H(&surface, dest, &settings);
      };
   }
   else if (target_format == Compression_format::BC7) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         bc7_enc_settings settings;
         GetProfile_slow(&settings);

         CompressBlocksBC7(&surface, dest, &settings);
      };
   }
   else if (target_format == Compression_format::BC7_ALPHA) {
      compressor = [](const rgba_surface surface, std::uint8_t* const dest) {
         bc7_enc_settings settings;
         GetProfile_alpha_slow(&settings);

         CompressBlocksBC7(&surface, dest, &settings);
      };
   }

   for (auto i = 0; i < dest_image.GetImageCount(); ++i) {
      const auto& src_sub_image = image.GetImages()[i];
      auto& dest_sub_image = dest_image.GetImages()[i];

      const auto work_count = dest_sub_image.height / 4;

      if (work_count) {
         std::for_each_n(std::execution::par, Index_iterator{}, work_count, [&](auto index) {
            rgba_surface surface;
            surface.ptr = src_sub_image.pixels + (index * 4 * src_sub_image.rowPitch);
            surface.width = gsl::narrow_cast<std::uint32_t>(src_sub_image.width);
            surface.height = 4;
            surface.stride = gsl::narrow_cast<std::uint32_t>(src_sub_image.rowPitch);

            auto* const dest =
               dest_sub_image.pixels + (index * dest_sub_image.rowPitch);

            compressor(surface, dest);
         });
      }
      else {
         const Image_span src_span{{src_sub_image.width, src_sub_image.height},
                                   src_sub_image.rowPitch,
                                   reinterpret_cast<std::byte*>(src_sub_image.pixels),
                                   src_sub_image.format};

         alignas(16) std::array<std::uint8_t, 64> storage;
         Image_span padded_span{{4, 4},
                                16,
                                reinterpret_cast<std::byte*>(storage.data()),
                                src_sub_image.format};

         for (auto y = 0; y < padded_span.size().y; ++y) {
            for (auto x = 0; x < padded_span.size().x; ++x) {
               padded_span.store({x, y}, src_span.load({x, y}));
            }
         }

         rgba_surface surface;
         surface.ptr = storage.data();
         surface.width = 4;
         surface.height = 4;
         surface.stride = 16;

         compressor(surface, dest_sub_image.pixels);
      }
   }

   if (config["sRGB"s].as<bool>(true)) {
      dest_image.OverrideFormat(DX::MakeSRGB(target_format_dxgi));
   }

   return dest_image;
}

}

auto process_image(const YAML::Node& config,
                   const std::filesystem::path& image_file_path) -> DX::ScratchImage
{
   Expects(fs::exists(image_file_path) && fs::is_regular_file(image_file_path));

   auto image = load_image(image_file_path, config["sRGB"s].as<bool>(true));
   const auto type = texture_type_from_string(config["Type"s].as<std::string>());

   if (type == Texture_type::passthrough) return image;

   if (type == Texture_type::cubemap) {
      image = fold_cubemap(std::move(image));
   }

   auto paired_image = load_paired_image(image_file_path.parent_path(), config);

   if (type == Texture_type::roughness) {
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
      if (type == Texture_type::normal_map)
         image = mipmap_normalmap(std::move(image));
      else
         image = mipmap_image(std::move(image));
   }

   if (type == Texture_type::roughness || type == Texture_type::metellic_roughness) {
      if (paired_image) {
         image = specular_anti_alias(std::move(image), std::move(*paired_image));
      }
      else {
         synced_print(
            "Image ", image_file_path,
            " is intended for use as a roughness map without "
            "a paired normal map reference, specular AA can not be "
            "applied without it. If this is intentional then ignore this.");
      }
   }

   if (!config["Uncompressed"s].as<bool>(false)) {
      image = compress_image(std::move(image), config);
   }

   return image;
}

}
