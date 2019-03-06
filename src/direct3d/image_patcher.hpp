#pragma once

#include "../core/shader_patch.hpp"
#include "upload_texture.hpp"

#include <memory>
#include <utility>

namespace sp::d3d9 {

class Image_patcher {
public:
   Image_patcher() = default;
   virtual ~Image_patcher() = default;

   Image_patcher(const Image_patcher&) = delete;
   Image_patcher& operator=(const Image_patcher&) = delete;
   Image_patcher(Image_patcher&&) = delete;
   Image_patcher& operator=(Image_patcher&&) = delete;

   virtual auto patch_image(const DXGI_FORMAT format, const UINT width,
                            const UINT height, const UINT mip_levels,
                            Upload_texture& input_texture) const noexcept
      -> std::pair<DXGI_FORMAT, std::unique_ptr<Upload_texture>> = 0;

   virtual auto map_dynamic_image(const DXGI_FORMAT format, const UINT width,
                                  const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture = 0;

   virtual void unmap_dynamic_image(core::Shader_patch& shader_patch,
                                    const core::Game_texture& texture,
                                    const UINT mip_level) noexcept = 0;

   virtual bool is_mapped() const noexcept = 0;
};

auto make_l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>;

auto make_a8l8_image_patcher() noexcept -> std::unique_ptr<Image_patcher>;

}
