#pragma once

#include <memory>

#include <DirectXTex.h>

namespace sp::d3d9 {

class Image_patcher {
public:
   Image_patcher() = default;
   virtual ~Image_patcher() = default;

   Image_patcher(const Image_patcher&) = delete;
   Image_patcher& operator=(const Image_patcher&) = delete;
   Image_patcher(Image_patcher&&) = delete;
   Image_patcher& operator=(Image_patcher&&) = delete;

   virtual auto create_image(const DXGI_FORMAT format, const UINT width,
                             const UINT height, const UINT mip_levels) const
      noexcept -> DirectX::ScratchImage = 0;

   virtual auto patch_image(const DirectX::ScratchImage& input_image) const
      noexcept -> DirectX::ScratchImage = 0;
};

auto make_l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>;

auto make_a8l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>;

}
