
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
         const auto src_resource = *input_image.GetImage(mip_level, 0, 0);
         auto dest_resource = *output_image.GetImage(mip_level, 0, 0);

         std::for_each_n(
            std::execution::par_unseq, Index_iterator{}, src_resource.height,
            [&](const int y) {
               const auto src_row =
                  gsl::span{src_resource.pixels + (src_resource.rowPitch * y),
                            static_cast<gsl::index>(src_resource.rowPitch)};
               auto dest_row =
                  gsl::span{dest_resource.pixels + (dest_resource.rowPitch * y),
                            static_cast<gsl::index>(dest_resource.rowPitch)};

               for (auto x = 0; x < src_resource.width; ++x) {
                  auto dest_pixel = dest_row.subspan(4 * x, 4);

                  dest_pixel[0] = dest_pixel[1] = dest_pixel[2] = src_row[x];
                  dest_pixel[3] = 0xff;
               }
            });
      }

      return output_image;
   }

private:
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
         const auto src_resource = *input_image.GetImage(mip_level, 0, 0);
         auto dest_resource = *output_image.GetImage(mip_level, 0, 0);

         std::for_each_n(
            std::execution::par_unseq, Index_iterator{}, src_resource.height,
            [&](const int y) {
               const auto src_row =
                  gsl::span{src_resource.pixels + (src_resource.rowPitch * y),
                            static_cast<gsl::index>(src_resource.rowPitch)};
               auto dest_row =
                  gsl::span{dest_resource.pixels + (dest_resource.rowPitch * y),
                            static_cast<gsl::index>(dest_resource.rowPitch)};

               for (auto x = 0; x < src_resource.width; ++x) {
                  const auto src_pixel = src_row.subspan(2 * x, 2);
                  auto dest_pixel = dest_row.subspan(4 * x, 4);

                  dest_pixel[0] = dest_pixel[1] = dest_pixel[2] = src_pixel[0];
                  dest_pixel[3] = src_pixel[1];
               }
            });
      }

      return output_image;
   }

private:
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
