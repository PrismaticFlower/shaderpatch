#pragma once

#include "../core/shader_patch.hpp"
#include "upload_texture.hpp"

#include <memory>
#include <utility>

namespace sp::d3d9 {

class Format_patcher {
public:
   Format_patcher() = default;
   virtual ~Format_patcher() = default;

   Format_patcher(const Format_patcher&) = delete;
   Format_patcher& operator=(const Format_patcher&) = delete;
   Format_patcher(Format_patcher&&) = delete;
   Format_patcher& operator=(Format_patcher&&) = delete;

   virtual auto patch_texture(const DXGI_FORMAT format, const UINT width,
                              const UINT height, const UINT mip_levels,
                              const UINT array_size, Upload_texture& input_texture) const
      noexcept -> std::pair<DXGI_FORMAT, std::unique_ptr<Upload_texture>> = 0;

   virtual auto map_dynamic_texture(const DXGI_FORMAT format, const UINT width,
                                    const UINT height, const UINT mip_level) noexcept
      -> core::Mapped_texture = 0;

   virtual void unmap_dynamic_texture(core::Shader_patch& shader_patch,
                                      const core::Game_texture& texture,
                                      const UINT mip_level) noexcept = 0;

   virtual bool is_mapped() const noexcept = 0;
};

auto make_l8_format_patcher() noexcept -> std::unique_ptr<Format_patcher>;

auto make_a8l8_format_patcher() noexcept -> std::unique_ptr<Format_patcher>;

}
