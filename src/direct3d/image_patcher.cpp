
#include "image_patcher.hpp"
#include "../logger.hpp"
#include "utility.hpp"

#include <array>
#include <execution>

#include <gsl/gsl>

#include <comdef.h>

namespace sp::d3d9 {

namespace {

class Image_patcher_l8 final : public Image_patcher {
public:
   auto create_image(const DXGI_FORMAT format, const UINT width,
                     const UINT height, const UINT mip_levels) const noexcept
      -> DirectX::ScratchImage override
   {
      Expects(format == DXGI_FORMAT_R8_UNORM);

      DirectX::ScratchImage image;
      image.Initialize2D(format, width, height, 2, mip_levels);

      return image;
   }

   auto patch_image(const DirectX::ScratchImage& input_image) const noexcept
      -> DirectX::ScratchImage override
   {
      Expects(input_image.GetMetadata().format == DXGI_FORMAT_R8_UNORM);

      const auto input_meta = input_image.GetMetadata();

      DirectX::ScratchImage output_image;

      if (const auto result =
             output_image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, input_meta.width,
                                       input_meta.height, 1, input_meta.mipLevels);
          FAILED(result)) {
         log_and_terminate("Failed to convert texture format! reason: ",
                           _com_error{result}.ErrorMessage());
      }

      for (auto mip_level = 0; mip_level < input_meta.mipLevels; ++mip_level) {
         patch_subimage(*input_image.GetImage(mip_level, 0, 0),
                        *output_image.GetImage(mip_level, 0, 0));
      }

      return output_image;
   }

   auto map_dynamic_image(const DXGI_FORMAT format, const UINT width,
                          const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture override
   {
      Expects(format == DXGI_FORMAT_R8_UNORM);
      Expects(!is_mapped());

      if (const auto result =
             _dynamic_patch_image.Initialize2D(format, width >> mip_level,
                                               height >> mip_level, 1, 1);
          FAILED(result)) {
         log_and_terminate(
            "Failed to create scratch image for dynamic image patcher.");
      }

      auto* const image = _dynamic_patch_image.GetImage(0, 0, 0);

      return {image->rowPitch, image->slicePitch,
              reinterpret_cast<std::byte*>(image->pixels)};
   }

   void unmap_dynamic_image(core::Shader_patch& shader_patch,
                            const core::Game_texture& texture,
                            const UINT mip_level) noexcept override
   {
      Expects(is_mapped());

      auto mapped = shader_patch.map_dynamic_texture(texture, mip_level,
                                                     D3D11_MAP_WRITE_DISCARD);

      const auto src_image = *_dynamic_patch_image.GetImage(0, 0, 0);

      DirectX::Image mapped_image;
      mapped_image.width = src_image.width;
      mapped_image.height = src_image.height;
      mapped_image.format = DXGI_FORMAT_R8G8B8A8_UNORM;
      mapped_image.rowPitch = mapped.row_pitch;
      mapped_image.slicePitch = mapped.depth_pitch;
      mapped_image.pixels = reinterpret_cast<std::uint8_t*>(mapped.data);

      patch_subimage(src_image, mapped_image);
      _dynamic_patch_image = DirectX::ScratchImage{};

      shader_patch.unmap_dynamic_texture(texture, mip_level);
   }

   bool is_mapped() const noexcept override
   {
      return _dynamic_patch_image.GetImageCount() != 0;
   }

private:
   static void patch_subimage(const DirectX::Image& source,
                              const DirectX::Image& dest) noexcept
   {
      Expects(source.format == DXGI_FORMAT_R8_UNORM);
      Expects(dest.format == DXGI_FORMAT_R8G8B8A8_UNORM);

      std::for_each_n(std::execution::par_unseq, Index_iterator{},
                      source.height, [&](const int y) {
                         const auto src_row =
                            gsl::span{source.pixels + (source.rowPitch * y),
                                      static_cast<gsl::index>(source.rowPitch)};
                         auto dest_row =
                            gsl::span{dest.pixels + (dest.rowPitch * y),
                                      static_cast<gsl::index>(dest.rowPitch)};

                         for (auto x = 0; x < source.width; ++x) {
                            auto dest_pixel = dest_row.subspan(4 * x, 4);

                            dest_pixel[0] = dest_pixel[1] = dest_pixel[2] =
                               src_row[x];
                            dest_pixel[3] = 0xff;
                         }
                      });
   }

   DirectX::ScratchImage _dynamic_patch_image;
};

class Image_patcher_a8l8 final : public Image_patcher {
public:
   auto create_image(const DXGI_FORMAT format, const UINT width,
                     const UINT height, const UINT mip_levels) const noexcept
      -> DirectX::ScratchImage override
   {
      Expects(format == DXGI_FORMAT_R8G8_UNORM);

      DirectX::ScratchImage image;
      image.Initialize2D(format, width, height, 2, mip_levels);

      return image;
   }

   auto patch_image(const DirectX::ScratchImage& input_image) const noexcept
      -> DirectX::ScratchImage override
   {
      Expects(input_image.GetMetadata().format == DXGI_FORMAT_R8G8_UNORM);

      const auto input_meta = input_image.GetMetadata();

      DirectX::ScratchImage output_image;

      if (const auto result =
             output_image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, input_meta.width,
                                       input_meta.height, 1, input_meta.mipLevels);
          FAILED(result)) {
         log_and_terminate("Failed to convert texture format! reason: ",
                           _com_error{result}.ErrorMessage());
      }

      for (auto mip_level = 0; mip_level < input_meta.mipLevels; ++mip_level) {
         patch_subimage(*input_image.GetImage(mip_level, 0, 0),
                        *output_image.GetImage(mip_level, 0, 0));
      }

      return output_image;
   }

   auto map_dynamic_image(const DXGI_FORMAT format, const UINT width,
                          const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture override
   {
      Expects(format == DXGI_FORMAT_R8G8_UNORM);
      Expects(!is_mapped());

      if (const auto result =
             _dynamic_patch_image.Initialize2D(format, width >> mip_level,
                                               height >> mip_level, 1, 1);
          FAILED(result)) {
         log_and_terminate(
            "Failed to create scratch image for dynamic image patcher.");
      }

      auto* const image = _dynamic_patch_image.GetImage(0, 0, 0);

      return {image->rowPitch, image->slicePitch,
              reinterpret_cast<std::byte*>(image->pixels)};
   }

   void unmap_dynamic_image(core::Shader_patch& shader_patch,
                            const core::Game_texture& texture,
                            const UINT mip_level) noexcept override
   {
      Expects(is_mapped());

      auto mapped = shader_patch.map_dynamic_texture(texture, mip_level,
                                                     D3D11_MAP_WRITE_DISCARD);

      const auto src_image = *_dynamic_patch_image.GetImage(0, 0, 0);

      DirectX::Image mapped_image;
      mapped_image.width = src_image.width;
      mapped_image.height = src_image.height;
      mapped_image.format = DXGI_FORMAT_R8G8B8A8_UNORM;
      mapped_image.rowPitch = mapped.row_pitch;
      mapped_image.slicePitch = mapped.depth_pitch;
      mapped_image.pixels = reinterpret_cast<std::uint8_t*>(mapped.data);

      patch_subimage(src_image, mapped_image);
      _dynamic_patch_image = DirectX::ScratchImage{};

      shader_patch.unmap_dynamic_texture(texture, mip_level);
   }

   bool is_mapped() const noexcept override
   {
      return _dynamic_patch_image.GetImageCount() != 0;
   }

private:
   static void patch_subimage(const DirectX::Image& source,
                              const DirectX::Image& dest) noexcept
   {
      Expects(source.format == DXGI_FORMAT_R8G8_UNORM);
      Expects(dest.format == DXGI_FORMAT_R8G8B8A8_UNORM);

      std::for_each_n(std::execution::par_unseq, Index_iterator{},
                      source.height, [&](const int y) {
                         const auto src_row =
                            gsl::span{source.pixels + (source.rowPitch * y),
                                      static_cast<gsl::index>(source.rowPitch)};
                         auto dest_row =
                            gsl::span{dest.pixels + (dest.rowPitch * y),
                                      static_cast<gsl::index>(dest.rowPitch)};

                         for (auto x = 0; x < source.width; ++x) {
                            const auto src_pixel = src_row.subspan(2 * x, 2);
                            auto dest_pixel = dest_row.subspan(4 * x, 4);

                            dest_pixel[0] = dest_pixel[1] = dest_pixel[2] =
                               src_pixel[0];
                            dest_pixel[3] = src_pixel[1];
                         }
                      });
   }

   DirectX::ScratchImage _dynamic_patch_image;
};

}

auto make_l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>
{
   return std::make_unique<Image_patcher_l8>();
}

auto make_a8l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>
{
   return std::make_unique<Image_patcher_a8l8>();
}

}
